/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include <time.h>
#include <linux/types.h>

/* Update publisher counter value and timestamp */
void updateLRMeasurementsPublisher(struct timespec start_time, uint64_t countValue);
/* Update subscriber counter value and timestamp */
void updateLRMeasurementsSubscriber(struct timespec receive_time, uint64_t countValue);

/* Write the computed RTT in multiple csvs */
void* fileWriteLatency(void *arg);

/* Compute the latency for RTT */
void* latency_computation(void *arg);

/* Print the queue depth */
void printQueueDepth(void);
