/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_ECOS

UA_DateTime UA_DateTime_now(void) 
{
	unsigned long long t_ms = ertec_localtime();	
	return (t_ms * UA_DATETIME_MSEC) + UA_DATETIME_UNIX_EPOCH;
}

/* Credit to https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c */
UA_Int64 UA_DateTime_localTimeUtcOffset(void){
  time64_t gmt, rawtime = ertec_time_s(NULL);
  struct tm *ptm;
  struct tm gbuf;
  ptm = gmtime64_r(&rawtime, &gbuf);
  // Request that mktime() looksup dst in timezone database
  ptm->tm_isdst = -1;
  gmt = rtc_update_day_of_week(ptm);
  return (UA_Int64) (difftime(rawtime, gmt) * UA_DATETIME_SEC);
}

UA_DateTime UA_DateTime_nowMonotonic(void) 
{
  unsigned long long t_ms = OsCurrentTime_ms(0);
  return (t_ms * UA_DATETIME_MSEC) + UA_DATETIME_UNIX_EPOCH;
}

#endif /* UA_ARCHITECTURE_ECOS */
