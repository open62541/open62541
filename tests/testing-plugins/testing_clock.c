/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_clock.h"
#include <time.h>

/* To avoid zero timestamp value in header, the testingClock
 * is assigned with non-zero timestamp to pass unit tests */
static UA_DateTime testingClock = 0x5C8F735D;

void
UA_fakeSleep(UA_UInt32 duration) {
    testingClock += duration * UA_DATETIME_MSEC;
}

UA_DateTime UA_DateTime_now_fake(UA_EventLoop *el) {
    return testingClock;
}
