/*
 * opcua_time.c
 *
 *  Created on: Feb 5, 2014
 *      Author: opcua
 */
#include "opcua_builtInDatatypes.h"
#include "opcua_advancedDatatypes.h"
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>


//get the current Server Time
UA_DateTime opcua_time_now()
{
	Int64 time1970 = 621356004000000000;

	time_t now = time(NULL);
	Int64 now_nano_ticks = ((Int64)now) * 10 * 1000 * 1000;
	return now_nano_ticks + time1970;

}


