/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_clock.h"

UA_DateTime testingClock = 0;

UA_DateTime UA_DateTime_now(void) {
    return testingClock;
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
    return testingClock;
}

void
UA_sleep(UA_DateTime duration) {
    testingClock += duration * UA_MSEC_TO_DATETIME;
}
