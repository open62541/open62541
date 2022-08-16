/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Stalder
 *    Copyright 2021 (c) Simon Kueppers, 2pi-Labs GmbH
 */

#ifdef UA_ARCHITECTURE_FREERTOSLWIP

#include <open62541/types.h>
#include <task.h>

#ifdef UA_ARCHITECTURE_FREERTOSLWIP_POSIX_CLOCK

UA_DateTime UA_DateTime_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * UA_DATETIME_SEC) + (tv.tv_usec * UA_DATETIME_USEC) + UA_DATETIME_UNIX_EPOCH;
}

/* Credit to https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64 UA_DateTime_localTimeUtcOffset(void) {
    time_t gmt, rawtime = time(NULL);
    struct tm *ptm;
    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);
    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);
    return (UA_Int64) (difftime(rawtime, gmt) * UA_DATETIME_SEC);
}

#else /* UA_ARCHITECTURE_FREERTOSLWIP_POSIX_CLOCK */

/* The current time in UTC time */
UA_DateTime UA_DateTime_now(void) {
  UA_DateTime microSeconds = ((UA_DateTime)xTaskGetTickCount()) * (1000000 / configTICK_RATE_HZ);
  return ((microSeconds / 1000000) * UA_DATETIME_SEC) + ((microSeconds % 1000000) * UA_DATETIME_USEC) + UA_DATETIME_UNIX_EPOCH;
}

/* Offset between local time and UTC time */
UA_Int64 UA_DateTime_localTimeUtcOffset(void) {
  return 0; //TODO: this is set to zero since UA_DateTime_now() is just the local time, and not UTC.
}

#endif /* UA_ARCHITECTURE_FREERTOSLWIP_POSIX_CLOCK */

/* CPU clock invariant to system time changes. Use only to measure durations,
 * not absolute time. */
UA_DateTime UA_DateTime_nowMonotonic(void) {
  return (((UA_DateTime)xTaskGetTickCount()) * 1000 / configTICK_RATE_HZ) * UA_DATETIME_MSEC;
}

#endif /* UA_ARCHITECTURE_FREERTOSLWIP */
