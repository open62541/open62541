/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2024 (c) Julian Weiß, PRIMES GmbH
 */

#include <open62541/types.h>

/* Note that the EventLoop plugin provides its own internal time source (which
 * is typically just the normal system time). All internal access to the time
 * source should be through the EventLoop. The below is therefore for developer
 * convenience to just use UA_DateTime_now(). */

#if defined(UA_ARCHITECTURE_ZEPHYR)

#include <time.h>
#include <sys/time.h>
#include <zephyr/kernel.h>

UA_DateTime
UA_DateTime_now(void) {
    struct timespec ts;
    sys_clock_gettime(CLOCK_REALTIME,&ts);
    UA_DateTime time= (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec*UA_DATETIME_NSEC) +
           UA_DATETIME_UNIX_EPOCH;
    return time;
}

/* Credit to
 * https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64
UA_DateTime_localTimeUtcOffset(void) {
    /*Zephyr does not provide any mechanism to 
    calculate local time(may change for zephyr 4.4 or newer).*/
    return 0;
}

UA_DateTime
UA_DateTime_nowMonotonic(void) {
    return k_uptime_get();
}

#endif
