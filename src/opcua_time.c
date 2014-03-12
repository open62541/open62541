/*
 * opcua_time.c
 *
 *  Created on: Feb 5, 2014
 *      Author: opcua
 *
 * code inspired by
 * http://stackoverflow.com/questions/3585583/convert-unix-linux-time-to-windows-filetime
 */
#include "opcua_builtInDatatypes.h"
#include "opcua_advancedDatatypes.h"
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

// number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define FILETIME_UNIXTIME_BIAS_SEC 11644473600LL
//
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

// IEC 62541-6 §5.2.2.5  A DateTime value shall be encoded as a 64-bit signed integer
// which represents the number of 100 nanosecond intervals since January 1, 1601 (UTC).

UA_DateTime opcua_getTime() {
	UA_DateTime dateTime;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	dateTime = (tv.tv_sec + FILETIME_UNIXTIME_BIAS_SEC)
			* HUNDRED_NANOSEC_PER_SEC + tv.tv_usec * HUNDRED_NANOSEC_PER_USEC;
	return dateTime;
}

