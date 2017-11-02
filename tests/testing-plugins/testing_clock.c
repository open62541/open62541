/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_clock.h"
#ifdef UA_ENABLE_MULTITHREADING
#include <time.h>
#endif

UA_DateTime testingClock = 0;

UA_DateTime UA_DateTime_now(void) {
    return testingClock;
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
    return testingClock;
}

void
UA_sleep(UA_UInt32 duration) {
    testingClock += duration * UA_MSEC_TO_DATETIME;
}

#define NANO_SECOND_MULTIPLIER 1000000 // 1 millisecond = 1,000,000 Nanoseconds
void
UA_realsleep(UA_UInt32 duration) {
#ifdef UA_ENABLE_MULTITHREADING
    struct timespec sleepValue;
    sleepValue.tv_sec = 0;
    sleepValue.tv_nsec = duration * NANO_SECOND_MULTIPLIER;
    nanosleep(&sleepValue, NULL);
#endif
}
