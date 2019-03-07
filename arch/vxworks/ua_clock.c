/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Stalder
 */

#ifdef UA_ARCHITECTURE_VXWORKS

#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <time.h>
#include <sys/time.h>

#include <open62541/types.h>

UA_DateTime UA_DateTime_now(void){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * UA_DATETIME_SEC) + (tv.tv_usec * UA_DATETIME_USEC) + UA_DATETIME_UNIX_EPOCH;
}

/* Credit to https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64 UA_DateTime_localTimeUtcOffset(void){
  time_t gmt, rawtime = time(NULL);

  struct tm *ptm;
  struct tm gbuf;
  ptm = gmtime_r(&rawtime, &gbuf);
  // Request that mktime() looksup dst in timezone database
  ptm->tm_isdst = -1;
  gmt = mktime(ptm);

  return (UA_Int64) (difftime(rawtime, gmt) * UA_DATETIME_SEC);
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
  struct timespec ts;
#if !defined(CLOCK_MONOTONIC_RAW)
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
  return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100);
}

#endif /* UA_ARCHITECTURE_VXWORKS */

