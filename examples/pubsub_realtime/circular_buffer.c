/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/types.h>
#include <unistd.h>

#include <open62541/types.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/log.h>

#include "circular_buffer.h"

#define MAX_MEASUREMENTS               1000000
#define MAX_MEASUREMENTS_FW            100000000
/* Number of packets to be written in one csv */
#define PACKETS_IN_A_CSV               100000
#define SECONDS                        1000 * 1000 * 1000
#define SECONDS_INCREMENT              1
#ifndef CLOCK_TAI
#define CLOCK_TAI                      11
#endif
#define CLOCKID                        CLOCK_TAI
#define NSEC_PER_SEC                   1000000000L

/* Function which converts timespec value to nano seconds value */
static inline long long timespec_to_ns(const struct timespec *ts)
{
    return ((long long) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

UA_Boolean runningFilethread = true;

struct timespec resultTime;
struct timespec csv_timestamp;
UA_UInt64 time_in_ns;
UA_Double finalTime;

pthread_mutex_t pub_lock_rw;
pthread_mutex_t sub_lock_rw;
pthread_mutex_t latency_lock_rw;

/* Variable for enqueue and dequeue of the samples */
static UA_UInt64 front_latency_copy   = 0;
static UA_UInt64 rear_latency         = 0;
static UA_UInt64 rear_latency_copy    = 0;
static UA_UInt64 rear_latency_copy_fw = 0;
static UA_UInt64 rear_latency_diff    = 0;

/* Variables to find the maximum queue depth */
static UA_UInt64 max_queue_depth_latency = 0;
static UA_UInt64 tmp_depth_latency       = 0;
static UA_UInt64 max_queue_depth_pub     = 0;
static UA_UInt64 tmp_depth_pub           = 0;
static UA_UInt64 max_queue_depth_sub     = 0;
static UA_UInt64 tmp_depth_sub           = 0;

/* File write in different csv files */
static UA_UInt64 filewrite_latency       = 0;
static UA_UInt64 csvCounter_latency      = 1;

UA_Boolean latencyWrite_flag;
UA_UInt64 missed_counter   = 0;
UA_UInt64 repeated_counter = 0;
size_t computational_index = 0;

struct publishMeasurement{
    /* Array to store published counter data */
    UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
    /* Array to store timestamp */
    struct timespec     publishTimestamp[MAX_MEASUREMENTS];
} pub;

size_t measurementsLRPublisher = 0;
size_t index_pub  = 0;

struct subscribeMeasurement{
    /* Array to store subscribed counter data */
    UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
    /* Array to store timestamp */
    struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];
} sub;

size_t measurementsLRSubscriber = 0;
size_t index_sub  = 0;

size_t latencyBlockMeasurements  = 0;
static char latency_measurements[MAX_MEASUREMENTS_FW];

/* Function which gives the time difference of the two time
 * result = stop - start */
static void
timespec_diff(struct timespec *start, struct timespec *stop,
              struct timespec *result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

/* Verify if the latency buffer gets filled */
static int isFull_latency(void) {
    /* Rear latency difference is the number of bytes that has been calculated and copied into the latency buffer at the previous cycle.
     * Assuming it will be the length of the next computation, check the latency buffer by adding the current latency pointer
     * with this length and determine the latency buffer is filled or not */
    if (front_latency_copy > rear_latency) {
        if ((rear_latency + rear_latency_diff) >= front_latency_copy)
            return 1;
        else
            return 0;
    }
    /* Handle the rare case scenario where latency read buffer is at the start and write buffer is at the
     * end of the circular buffer. */
    else {
        if (((rear_latency + rear_latency_diff) >= MAX_MEASUREMENTS_FW) && (rear_latency > front_latency_copy) && \
            (((rear_latency + rear_latency_diff) % MAX_MEASUREMENTS_FW) >= front_latency_copy))
           return 1;
        else
           return 0;
    }
}

/* Circular buffer for the publisher is full */
static int isFull_publisher(void)
{
    /* measurementsLRPublishers is the write pointer in publisher circular queue and index_pub is the read pointer in publisher circular queue
     * Verify write pointer overwrites with the read pointer which indicates the publisher queue full */
    if (measurementsLRPublisher+1 == index_pub)
        return 1;

    if (measurementsLRPublisher+1 > MAX_MEASUREMENTS && index_pub == 0)
        return 1;

    return 0;
}

/* Circular buffer for the subscriber is full */
static int isFull_subscriber(void)
{
    /* measurementsLRSubscriber is the write pointer in subscriber circular queue and index_pub is the read pointer in subscriber circular queue
     * Verify write pointer overwrites with the read pointer which indicates the subscriber queue full */
    if (measurementsLRSubscriber+1 == index_sub)
        return 1;

    if (measurementsLRSubscriber+1 > MAX_MEASUREMENTS && index_sub == 0)
        return 1;

    return 0;
}

/* Circular queue of the latency is empty */
static int isEmpty_latency(void)
{
    /* Verify whether the latency queue is empty - write and read pointer in the same place */
    if(front_latency_copy == rear_latency)
        return 1;

    return 0;
}

void
updateLRMeasurementsPublisher(struct timespec start_time, UA_UInt64 countValue) {
    /* Check whether the publisher circular queue is full */
    if (isFull_publisher()) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Publisher Queue is full");
        runningFilethread = false;
        printQueueDepth();
        exit(1); // Exiting the application if publisher queue is full
    }

    /* To find the maximum queue depth - This determines how much user can provide maximum queue length to the circular buffer */
    /* It is checked because to find out whether the circular queue starts the next queue */
    if (measurementsLRPublisher >= index_pub) { /* TODO: Solve this check. This should be > alone. changing it to >= for first packet */
        pthread_mutex_lock(&pub_lock_rw);
        /* Subtract measurementsLRPublisher and index_pub to get queue depth */
        tmp_depth_pub = measurementsLRPublisher - index_pub;
        if (tmp_depth_pub > max_queue_depth_pub) {
            /* Store queue depth in a temporary variable to compare and update new queue depth */
            max_queue_depth_pub = tmp_depth_pub;
        }
        pthread_mutex_unlock(&pub_lock_rw);
    }
    else {
        /* Check for negative scenarios when measurementsLRPublisher is lesser than index_pub */
        pthread_mutex_lock(&pub_lock_rw);
        tmp_depth_pub = (MAX_MEASUREMENTS - index_pub) + measurementsLRPublisher;
        if (tmp_depth_pub > max_queue_depth_pub) {
            max_queue_depth_pub = tmp_depth_pub;
        }

        pthread_mutex_unlock(&pub_lock_rw);
    }

    /* Pass the time value to timestamp in publisher structure
     * Pass the counter value to countervalue in publisher structure */
    pub.publishTimestamp[measurementsLRPublisher]    = start_time;
    pub.publishCounterValue[measurementsLRPublisher] = countValue;
    measurementsLRPublisher++;

    if (measurementsLRPublisher == MAX_MEASUREMENTS) {
        /* Reset the array index after MAX_MEASUREMENTS reached to start from initial pointer of the circular buffer */
        measurementsLRPublisher = 0;
    }
}

void
updateLRMeasurementsSubscriber(struct timespec start_time, UA_UInt64 countValue) {
    /* Check whether the subscriber circular queue is full */
    if (isFull_subscriber()) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Subscriber Queue is full");
        runningFilethread = false;
        printQueueDepth();
        exit(1); // Exiting the application if subscriber queue is full
    }

    /* To find the maximum queue depth - This determines how much user can provide maximum queue lenght to the circular buffer */
    /* It is checked because to find out whether the circular queue starts the next queue */
    if (measurementsLRSubscriber >= index_sub) { /* TODO: Solve this check. This should be > alone. changing it to >= for first packet */
        pthread_mutex_lock(&sub_lock_rw);
        /* Subtract measurementsLRSubscriber and index_sub to get queue depth */
        tmp_depth_sub = measurementsLRSubscriber - index_sub;
        if (tmp_depth_sub > max_queue_depth_sub) {
            /* Store queue depth in a temporary variable to compare and update new queue depth (for user to update MAX_MEASUREMENTS value)*/
            max_queue_depth_sub = tmp_depth_sub;
        }
        pthread_mutex_unlock(&sub_lock_rw);
    }
    /* Check for negative scenarios when measurementsLRPublisher is lesser than index_pub */
    else {
        pthread_mutex_lock(&sub_lock_rw);
        tmp_depth_sub = (MAX_MEASUREMENTS - index_sub) + measurementsLRSubscriber;
        if (tmp_depth_sub > max_queue_depth_sub) {
            max_queue_depth_sub = tmp_depth_sub;
        }
        pthread_mutex_unlock(&sub_lock_rw);
    }

    /* Pass the time value to timestamp in susbcriber structure
     * Pass the counter value to countervalue in susbcriber structure */
    sub.subscribeTimestamp[measurementsLRSubscriber] = start_time;
    sub.subscribeCounterValue[measurementsLRSubscriber] = countValue;
    measurementsLRSubscriber++;

    if (measurementsLRSubscriber == MAX_MEASUREMENTS) {
        /* Reset the array index after MAX_MEASUREMENTS reached to start from initial pointer of the circular buffer */
        measurementsLRSubscriber = 0;
    }
}

/* Timespec nano second field conversion - It protects the overflow of the nanosecond value in the
 * timespec structure normalize the timespec value */
static void nsFieldConversion(struct timespec *timeSpecValue) {
    /* Check if ns field is greater than '1 ns less than 1sec' */
    while (timeSpecValue->tv_nsec > (SECONDS -1)) {
        /* Move to next second and remove it from ns field */
        timeSpecValue->tv_sec  += SECONDS_INCREMENT;
        timeSpecValue->tv_nsec -= SECONDS;
    }

}

/* Thread to compute the RTT latency between T1 and T8 */
void* latency_computation(void *arg) {
    /* Local buffer to hold the computed latency buffer for a single cycle when the circular buffer
     * gets filled for every rotation */
    char local_buffer[100];
    UA_UInt64 index = 0;
    struct timespec nextnanosleeptime_latency;
    /* Obtain the interval from the thread argument */
    UA_UInt64 *interval_ns = (UA_UInt64 *)arg;

    if (pthread_mutex_init(&latency_lock_rw, NULL) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Mutex init has failed");
        exit(1);
    }

    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime_latency);

    /* Variable to nano Sleep - start computation after 7 seconds as publisher and subscriber
     * starts at 5 seconds. Provide time lapse for publisher and subscriber to transfer packets */
    nextnanosleeptime_latency.tv_sec  += 7;
    nextnanosleeptime_latency.tv_nsec = (__syscall_slong_t)(((*(UA_Double *)interval_ns)) * 0.6);
    nsFieldConversion(&nextnanosleeptime_latency);

    while(runningFilethread)
    {
        UA_UInt64 local_buffer_count = 0;
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime_latency, NULL);

        /* Check whether the latency queue is full */
        if (isFull_latency()) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Latency queue is full!");
            exit(1);
        }

        /* Check whether both publisher and subscriber has been started for latency computation */
        if((pub.publishCounterValue[index_pub] == 0) &&
            (sub.subscribeCounterValue[index_sub] == 0))
            continue;

        /* Compute RTT latency by equating counter values - Counter values same for both publisher queue and subscriber queue
         * index_pub and index_sub will be incremented as a packet is successfully transmitted and received */
        if (pub.publishCounterValue[index_pub] == sub.subscribeCounterValue[index_sub]) {
            clock_gettime(CLOCKID, &csv_timestamp);
            timespec_diff(&pub.publishTimestamp[index_pub], &sub.subscribeTimestamp[index_sub], &resultTime);
            time_in_ns = (UA_UInt64)timespec_to_ns(&resultTime);
            finalTime = ((UA_Double)(time_in_ns))/1000;
            index_pub++;
            index_sub++;
        }
        /* Check for the missed counters - Counter value for the publisher queue is lesser than the counter value of the subscriber queue
         * Missed counter has been incremented. Only index_pub will be incremented as the next packet will be mapped with the current value of the subscriber queue */
        else if(pub.publishCounterValue[index_pub] < sub.subscribeCounterValue[index_sub]) {
            clock_gettime(CLOCKID, &csv_timestamp);
            timespec_diff(&pub.publishTimestamp[index_pub], &sub.subscribeTimestamp[index_sub], &resultTime);
            time_in_ns = (UA_UInt64)timespec_to_ns(&resultTime);
            finalTime = ((UA_Double)(time_in_ns))/1000;
            missed_counter++;
            index_pub++;
        }
        /* Check for the repeated counters - Counter value for the susbcriber queue is lesser than the counter value of the publisher queue
         * Further check whether the previous subcriber counter value is the same for this subscriber counter value
         * If it is, repeated counter has been incremented. Only index_sub will be incremented as the next packet
         * will be mapped with the current value of the publisher queue */
        else {
            if (sub.subscribeCounterValue[index_sub - 1] == sub.subscribeCounterValue[index_sub])
                repeated_counter++;

            clock_gettime(CLOCKID, &csv_timestamp);
            timespec_diff(&pub.publishTimestamp[index_pub], &sub.subscribeTimestamp[index_sub], &resultTime);
            time_in_ns = (UA_UInt64)timespec_to_ns(&resultTime);
            finalTime = ((UA_Double)(time_in_ns))/1000;
            index_sub++;
        }

        /* Write the current time, latency value, missed counters and repeated counters to the latency circular queue using sprintf function */
        if (((rear_latency - rear_latency_copy) + rear_latency) < MAX_MEASUREMENTS_FW) {
            rear_latency_copy = rear_latency;
            rear_latency += (UA_UInt64)sprintf(&latency_measurements[rear_latency],
                                               "%ld.%09ld, %0.3f, %ld, %ld\n",
                                               csv_timestamp.tv_sec, csv_timestamp.tv_nsec,
                                               finalTime, missed_counter, repeated_counter);
            computational_index++;
            rear_latency_diff = (rear_latency - rear_latency_copy);
        }
        else {
            /* If the circular buffer reaches the end of the buffer, then it cannot be handled with a simple sprintf that may lead to unallocated
             * memory which leads to seg fault. So there needs a separation in the copy of the buffer i.e., the buffer to be copied into the circular buffer
             * will be stored into the local buffer at first. This buffer will be copied to the circular buffer up to the end of MAX_MEASUREMENTS.
             * Then the remaining buffer will be copied into the start of the the circular buffer from the index 0 */
            local_buffer_count += (UA_UInt64)sprintf(&local_buffer[local_buffer_count],
                                                     "%ld.%09ld, %0.3f, %ld, %ld\n",
                                                     csv_timestamp.tv_sec, csv_timestamp.tv_nsec,
                                                     finalTime, missed_counter, repeated_counter);
            for (index = 0; index < local_buffer_count; index++) {
                if (rear_latency < MAX_MEASUREMENTS_FW) {
                    latency_measurements[rear_latency] = local_buffer[index];
                    rear_latency++;
                }
                else
                    break;
            }

            rear_latency_diff = ((rear_latency + local_buffer_count) - rear_latency_copy);
            rear_latency      = 0;
            rear_latency_copy = 0;
            while(index < local_buffer_count) {
                latency_measurements[rear_latency] = local_buffer[index];
                rear_latency++;
                index++;
            }

            computational_index++;
        }

        /* Create multiple csvs to write latency values, i.e., each csv with PACKETS_IN_A_CSV count
         * For every filewrite_latency enabled by the number of packets with max measurements filewrite,
         * packets of PACKETS_IN_A_CSV will be written into the csv file */
        if ((csvCounter_latency * PACKETS_IN_A_CSV) <= computational_index) {
            filewrite_latency = rear_latency;
            csvCounter_latency++;
        }

        /* Queue depth calculation - This determines how much user can provide maximum queue length to the circular buffer */
        if (rear_latency > front_latency_copy) {
            pthread_mutex_lock(&latency_lock_rw);
            /* Subtract rear_sub and front_sub to get queue depth */
            tmp_depth_latency = rear_latency - front_latency_copy;
            if (tmp_depth_latency > max_queue_depth_latency)
                max_queue_depth_latency = tmp_depth_latency;

            pthread_mutex_unlock(&latency_lock_rw);
        }
        else {
            /* As the enqueue is in next cycle, rear_sub is added with MAX_MEASUREMENTS
               and subtracting it with front_sub */
            pthread_mutex_lock(&latency_lock_rw);
            tmp_depth_latency = (rear_latency + MAX_MEASUREMENTS_FW) - front_latency_copy;
            if (tmp_depth_latency > max_queue_depth_latency)
                max_queue_depth_latency = tmp_depth_latency;

            pthread_mutex_unlock(&latency_lock_rw);
        }

        if (index_pub == MAX_MEASUREMENTS)
            index_pub = 0; /* Reset the array index after MAX_MEASUREMENTS is reached */

        if (index_sub == MAX_MEASUREMENTS)
            index_sub = 0; /* Reset the array index after MAX_MEASUREMENTS is reached */

        nextnanosleeptime_latency.tv_nsec += (__syscall_slong_t)*interval_ns;
        nsFieldConversion(&nextnanosleeptime_latency);
    }

    UA_free(interval_ns);
    return (void*)NULL;
}

void* fileWriteLatency(void *arg)
{
    UA_UInt64 csvCount = 0;
    UA_UInt64 write_count_latency = 0;

    /* Wait for the other threads to start - Once the latency has been computed, write
     * the computed values to the files */
    sleep(8);
    while(runningFilethread)
    {
        FILE *fp_capture;
        /* For every PACKETS_IN_A_CSV packets, filewrite_latency will be changed and new csv file will be created for writing into it */
        if (rear_latency_copy_fw != filewrite_latency) {
            char csvNameLatency[255];
            snprintf(csvNameLatency, 255, "latency_%09ld.csv", csvCount);
            char *fileRTTData = csvNameLatency;
            fp_capture = fopen(fileRTTData, "w");
            csvCount++;
            latencyWrite_flag = 1;
        }

        /* Check whether latency queue is empty */
        if(isEmpty_latency())
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Latency Queue is empty");
        else {
            /* Check whether enqueue and dequeue variable of latency buffer are equal */
            if (front_latency_copy == rear_latency) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Resetting the publisher queue");
                printQueueDepth();
                exit(1);
            }
            else
            {
                if(latencyWrite_flag == 1) {
                    rear_latency_copy_fw  = filewrite_latency;
                    /* To find the maximum queue depth - This determines how much user can provide maximum queue length to the circular buffer */
                    /* It is checked because to find out whether the circular queue starts the next queue */
                    if (rear_latency_copy_fw > front_latency_copy) {
                        pthread_mutex_lock(&latency_lock_rw);
                        /* Subtract rear_pub and front_pub to get queue depth */
                        tmp_depth_latency = rear_latency_copy_fw - front_latency_copy;
                        if (tmp_depth_latency > max_queue_depth_latency)
                            max_queue_depth_latency = tmp_depth_latency;

                        pthread_mutex_unlock(&latency_lock_rw);
                    }
                    else {
                        pthread_mutex_lock(&latency_lock_rw);
                        /* As the enqueue is in next cycle, rear_pub is added with MAX_MEASUREMENTS_FW
                         * and subtracting it with front_pub */
                        tmp_depth_latency = (rear_latency_copy_fw + MAX_MEASUREMENTS_FW) - front_latency_copy;
                        if (tmp_depth_latency > max_queue_depth_latency)
                            max_queue_depth_latency = tmp_depth_latency;

                        pthread_mutex_unlock(&latency_lock_rw);
                    }

                    /* Write the queued PACKETS_IN_A_CSV packets to the created csv file */
                    if (rear_latency_copy_fw > front_latency_copy) {
                        write_count_latency = rear_latency_copy_fw - front_latency_copy;
                        fwrite(&latency_measurements[front_latency_copy], write_count_latency, 1, fp_capture);
                        front_latency_copy = rear_latency_copy_fw;
                    }
                    /* If the queued packets are separated by the circular buffer like some buffer at the last and the remaining at the start,
                     * then the file write will proceed accordingly by writing the end buffer of the circular queue at first and then
                     * writing the remaining to that csv file */
                    else {
                       write_count_latency = MAX_MEASUREMENTS_FW - front_latency_copy;
                       fwrite(&latency_measurements[front_latency_copy], write_count_latency, 1, fp_capture);
                       front_latency_copy = 0;
                       fwrite(&latency_measurements[front_latency_copy], rear_latency_copy_fw, 1, fp_capture);
                       front_latency_copy = rear_latency_copy_fw;
                    }

                    latencyWrite_flag = 0;
                    /* Close the written csv file */
                    fclose(fp_capture);
                }
            }
        }

        /* Sleep for 1ms and and check for the latency computed values to write into the csv */
        usleep(1000);
    }

    return (void*)NULL;
}

/* Print the queue depth of the publisher and subscriber */
void printQueueDepth() {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Max publisher queue depth: %ld", max_queue_depth_pub);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Max subscriber queue depth: %ld", max_queue_depth_sub);
    runningFilethread = false;
}
