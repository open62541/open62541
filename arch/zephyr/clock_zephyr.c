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
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * UA_DATETIME_SEC) + (tv.tv_usec * UA_DATETIME_USEC) +
           UA_DATETIME_UNIX_EPOCH;
}

/* Credit to
 * https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64
UA_DateTime_localTimeUtcOffset(void) {
    time_t rawtime = time(NULL);
    struct tm *ptm = gmtime(&rawtime);
    /* Request mktime() to look up dst in timezone database */
    ptm->tm_isdst = -1;
    time_t gmt = mktime(ptm);
    return (UA_Int64)(difftime(rawtime, gmt) * UA_DATETIME_SEC);
}

UA_DateTime
UA_DateTime_nowMonotonic(void) {
    return k_uptime_get();
}

#endif
