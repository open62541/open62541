/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include <time.h>
#include <linux/types.h>

/* Update publisher counter value and timestamp */
void updateLRMeasurementsPublisher(struct timespec start_time, uint64_t countValue, struct timespec pub_time, struct timespec user_time, struct timespec subscriber_thread_jitter, struct timespec publisher_thread_jitter, struct timespec user_thread_jitter);
/* Update subscriber counter value and timestamp */
void updateLRMeasurementsSubscriber(struct timespec receive_time, uint64_t countValue);
void updateLRMeasurementsPublisherlb(struct timespec pub_time, struct timespec user_time, struct timespec subscriber_thread_jitter, struct timespec publisher_thread_jitter, struct timespec user_thread_jitter);

/* Write the computed RTT in multiple csvs publisher side */
void* fileWriteLatency(void *arg);

/* Write the computed RTT in multiple csvs loop back side */
void* fileWriteLatencylb(void *arg);

/* Compute the latency for RTT publisher side*/
void* latency_computation(void *arg);

/* Compute the latency for RTT loopback side*/
void* latency_computation_lb(void *arg);

void printQueueDepth(void);
