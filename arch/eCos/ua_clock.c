/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_ECOS

#include <open62541/types.h>

UA_DateTime UA_DateTime_now(void) {
  return UA_DateTime_nowMonotonic();
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

  cyg_tick_count_t TaskTime = cyg_current_time();

  UA_DateTimeStruct UATime;
  UATime.milliSec = (UA_UInt16) TaskTime;
  struct timespec ts;
  ts.tv_sec = UATime.milliSec / 1000;
  ts.tv_nsec = (UATime.milliSec % 1000) * 1000000;
  return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100) + UA_DATETIME_UNIX_EPOCH;
}

#endif /* UA_ARCHITECTURE_ECOS */
