/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SERVER_DEPRECATED_H_
#define UA_SERVER_DEPRECATED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"

/**
 * Deprecated API
 * ==============
 * This file contains outdated API definitions that are kept for backwards
 * compatibility. Please switch to the new API, as the following definitions
 * will be removed eventually. */

/**
 * UA_Job API
 * ----------
 * UA_Job was replaced since it unneccessarily exposed server internals to the
 * end-user. Please use plain UA_ServerCallbacks instead. The following UA_Job
 * definition contains just the fraction of the original struct that was useful
 * to end-users. */

typedef enum {
    UA_JOBTYPE_METHODCALL
} UA_JobType;

typedef struct {
    UA_JobType type;
    union {
        struct {
            void *data;
            UA_ServerCallback method;
        } methodCall;
    } job;
} UA_Job;

UA_DEPRECATED static UA_INLINE UA_StatusCode
UA_Server_addRepeatedJob(UA_Server *server, UA_Job job,
                         UA_UInt32 interval, UA_Guid *jobId) {
    return UA_Server_addRepeatedCallback(server, job.job.methodCall.method,
                                         job.job.methodCall.data, interval,
                                         (UA_UInt64*)(uintptr_t)jobId);
}

UA_DEPRECATED static UA_INLINE UA_StatusCode
UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId) {
    return UA_Server_removeRepeatedCallback(server, *(UA_UInt64*)(uintptr_t)&jobId);
}

#ifdef __cplusplus
}
#endif

#endif /* UA_SERVER_DEPRECATED_H_ */
