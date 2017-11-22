/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Enable POSIX features */
#ifndef _XOPEN_SOURCE
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
#include "testing_clock.h"

#ifdef _WIN32
#include <windows.h>	/* WinAPI */
#endif

UA_DateTime testingClock = 0;

UA_DateTime UA_DateTime_now(void) {
    return testingClock;
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
    return testingClock;
}

void
UA_fakeSleep(UA_UInt32 duration) {
    testingClock += duration * UA_MSEC_TO_DATETIME;
}

/* 1 millisecond = 1,000,000 Nanoseconds */
#define NANO_SECOND_MULTIPLIER 1000000

#ifdef _WIN32

void
UA_realSleep(UA_UInt32 duration) {
    Sleep(duration);
}

#else
void
UA_realSleep(UA_UInt32 duration) {
    UA_UInt32 sec = duration / 1000;
    UA_UInt32 ns = (duration % 1000) * NANO_SECOND_MULTIPLIER;
    struct timespec sleepValue;
    sleepValue.tv_sec = sec;
    sleepValue.tv_nsec = ns;
    nanosleep(&sleepValue, NULL);
}
#endif
