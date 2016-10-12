/*
 * Copyright (C) 2016 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#include "ua_config.h"
#include "ua_statuscode_msg.h"
#include "ua_util.h"

/*

 From generated file:

static const struct UA_StatusCode_msg_info UA_StatusCode_msg_table[] =
{
		{UA_STATUSCODE_GOOD, "EPERM", "Not owner"},
		{UA_STATUSCODE_BADINVALIDARGUMENT, "EPERM", "Not owner"},
		{UA_STATUSCODE_BADOUTOFMEMORY, "EPERM", "Not owner"}
};

static const unsigned int UA_StatusCode_msg_table_size = 0;
 */

UA_EXPORT const char* UA_StatusCode_msg(UA_StatusCode code) {
	for (unsigned int i=0; i<UA_StatusCode_msg_table_size; i++) {
		if (UA_StatusCode_msg_table[i].value == code)
			return UA_StatusCode_msg_table[i].msg;
	}
	return NULL;
}


UA_EXPORT const char* UA_StatusCode_name(UA_StatusCode code) {
	for (unsigned int i=0; i<UA_StatusCode_msg_table_size; i++) {
		if (UA_StatusCode_msg_table[i].value == code)
			return UA_StatusCode_msg_table[i].name;
	}
	return NULL;
}


UA_StatusCode UA_EXPORT UA_StatusCode_from_name(const char* name) {
	if (name != NULL) {
		for (unsigned int i=0; i<UA_StatusCode_msg_table_size; i++) {
			if (strcmp(UA_StatusCode_msg_table[i].name, name) == 0)
				return UA_StatusCode_msg_table[i].value;
		}
	}
	return UA_UINT32_MAX;
}