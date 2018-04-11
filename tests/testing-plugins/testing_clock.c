/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <time.h>
#ifdef _WIN32
# include <windows.h>
#endif
#include "testing_clock.h"


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
