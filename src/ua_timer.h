/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_TIMER_H_
#define UA_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util.h"
#include "ua_job.h"

typedef void
(*UA_RepeatedJobsListProcessCallback)(void *processContext, UA_Job *job);

typedef struct {
    /* The linked list of jobs is sorted according to the execution timestamp. */
    SLIST_HEAD(RepeatedJobsSList, UA_RepeatedJob) repeatedJobs;

    /* Repeated jobs that shall be added or removed from the sorted list (with
     * atomic operations) */
    SLIST_HEAD(RepeatedJobsSList2, UA_RepeatedJob) addRemoveJobs;

    /* The callback to process jobs that have timed out */
    UA_RepeatedJobsListProcessCallback processCallback;
    void *processContext;
} UA_RepeatedJobsList;

void
UA_RepeatedJobsList_init(UA_RepeatedJobsList *rjl,
                         UA_RepeatedJobsListProcessCallback processCallback,
                         void *processContext);

UA_StatusCode
UA_RepeatedJobsList_addRepeatedJob(UA_RepeatedJobsList *rjl, const UA_Job job,
                                   const UA_UInt32 interval, UA_Guid *jobId);

UA_StatusCode
UA_RepeatedJobsList_removeRepeatedJob(UA_RepeatedJobsList *rjl, const UA_Guid jobId);

/* Process the repeated jobs that have timed out.
 * Returns the timestamp of the next scheduled repeated job. */
UA_DateTime
UA_RepeatedJobsList_process(UA_RepeatedJobsList *rjl, UA_DateTime nowMonotonic,
                            UA_Boolean *dispatched);

void
UA_RepeatedJobsList_deleteMembers(UA_RepeatedJobsList *rjl);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_TIMER_H_ */
