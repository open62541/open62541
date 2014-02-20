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
UA_DateTime opcua_getTime()
{
	//TODO call System Time function
	UA_DateTime time = 10000000;
	return time;
}


