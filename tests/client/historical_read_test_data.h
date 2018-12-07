/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

/* Data in this header is gathered from OPC Unified Architecture, Part 11, Release
1.03 Page 5-6 from OPC Foundation */

#ifndef UA_HISTORICAL_READ_TEST_DATA_H_
#define UA_HISTORICAL_READ_TEST_DATA_H_

#include "ua_types.h"

#define TESTDATA_START_TIME 99
#define TESTDATA_STOP_TIME 501
static UA_DateTime testData[] = {
    100,
    200,
    300,
    400,
    500
};

#endif /*UA_HISTORICAL_READ_TEST_DATA_H_*/
