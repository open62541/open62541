 /*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
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

#ifndef UA_JOB_H_
#define UA_JOB_H_

#include "ua_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UA_Server;
typedef struct UA_Server UA_Server;

typedef void (*UA_ServerCallback)(UA_Server *server, void *data);

/* Jobs describe work that is executed once or repeatedly in the server */
typedef struct {
    enum {
        UA_JOBTYPE_NOTHING,
        UA_JOBTYPE_DETACHCONNECTION, /* Detach the connection from the secure channel (but don't delete it) */
        UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER, /* The binary message is memory managed by the networklayer */
        UA_JOBTYPE_BINARYMESSAGE_ALLOCATED, /* The binary message was relocated away from the networklayer */
        UA_JOBTYPE_METHODCALL, /* Call the method as soon as possible */
        UA_JOBTYPE_METHODCALL_DELAYED, /* Call the method as soon as all previous jobs have finished */
    } type;
    union {
        UA_Connection *closeConnection;
        struct {
            UA_Connection *connection;
            UA_ByteString message;
        } binaryMessage;
        struct {
            void *data;
            UA_ServerCallback method;
        } methodCall;
    } job;
} UA_Job;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_JOB_H_ */
