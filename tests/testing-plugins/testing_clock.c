/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_clock.h"
#include <time.h>

UA_DateTime testingClock = 0;

UA_DateTime UA_DateTime_now(void) {
    return testingClock;
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
    return testingClock;
}

UA_DateTime UA_DateTime_localTimeUtcOffset(void) {
    return 0;
}

void
UA_fakeSleep(UA_UInt32 duration) {
    testingClock += duration * UA_DATETIME_MSEC;
}

/* 1 millisecond = 1,000,000 Nanoseconds */
#define NANO_SECOND_MULTIPLIER 1000000

void
UA_realSleep(UA_UInt32 duration) {
#ifdef _WIN32
    Sleep(duration);
#else
    UA_UInt32 sec = duration / 1000;
    UA_UInt32 ns = (duration % 1000) * NANO_SECOND_MULTIPLIER;
    struct timespec sleepValue;
    sleepValue.tv_sec = sec;
    sleepValue.tv_nsec = ns;
    nanosleep(&sleepValue, NULL);
#endif
}

void
UA_comboSleep(size_t duration) {
    UA_fakeSleep((UA_UInt32)duration);
    UA_realSleep((UA_UInt32)duration);
}
