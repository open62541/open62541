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

#ifndef UA_STATUSCODE_MSG_H_
#define UA_STATUSCODE_MSG_H_

#include "ua_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UA_StatusCode_msg_info {
	UA_StatusCode value;        /* The numeric value from UA_StatusCode */
	const char* name;    /* The equivalent symbolic value */
	const char* msg;    /* Short message about this value */
};

extern const struct UA_StatusCode_msg_info UA_StatusCode_msg_table[];

extern const unsigned int UA_StatusCode_msg_table_size;

UA_EXPORT const char * UA_StatusCode_msg(UA_StatusCode code);
UA_EXPORT const char * UA_StatusCode_name(UA_StatusCode code);
UA_StatusCode UA_EXPORT UA_StatusCode_from_name(const char* name);


#ifdef __cplusplus
}
#endif

#endif /* UA_STATUSCODE_MSG_H_ */
