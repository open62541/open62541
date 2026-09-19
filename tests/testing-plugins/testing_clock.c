/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_clock.h"
#include <time.h>

/* To avoid zero timestamp value in header, the testingClock
 * is assigned with non-zero timestamp to pass unit tests */
static UA_atomic(UA_DateTime) testingClock = 0x5C8F735D;

void
UA_fakeSleep(UA_UInt32 duration) {
#if UA_MULTITHREADING >= 100 && defined(_MSC_VER)
    /* The generic MSVC atomic helpers only support pointer-sized values. */
    InterlockedExchangeAdd64(&testingClock, duration * UA_DATETIME_MSEC);
#else
    UA_DateTime old = UA_atomic_load(&testingClock);
    UA_DateTime expected;
    do {
        expected = old;
        UA_atomic_cmpxchg(&testingClock, &old, expected + duration * UA_DATETIME_MSEC);
    } while(old != expected);
#endif
}

UA_DateTime UA_DateTime_now_fake(UA_EventLoop *el) {
#if UA_MULTITHREADING >= 100 && defined(_MSC_VER)
    return InterlockedCompareExchange64(&testingClock, 0, 0);
#else
    return UA_atomic_load(&testingClock);
#endif
}
