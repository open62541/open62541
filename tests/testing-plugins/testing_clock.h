/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_CLOCK_H_
#define TESTING_CLOCK_H_

#include <open62541/types.h>

/* The testing clock is used for reproducible unit tests that require precise
 * timings. It implements the following functions from ua_types.h. They return a
 * deterministic time that can be advanced manually with UA_sleep.
 *
 * UA_DateTime UA_EXPORT UA_DateTime_now(void);
 * UA_DateTime UA_EXPORT UA_DateTime_nowMonotonic(void); */

/* Forwards the testing clock by the given duration in ms */
void UA_fakeSleep(UA_UInt32 duration);

/* Sleep for the duration in milliseconds. Used to wait for workers to complete. */
void UA_realSleep(UA_UInt32 duration);

#endif /* TESTING_CLOCK_H_ */
