/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/


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
