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

#include <open62541/types.h>

#include <limits.h>

#define TESTDATA_START_TIME 1
#define TESTDATA_STOP_TIME 601
static UA_DateTime testData[] = {
    100,
    200,
    300,
    400,
    500
};
static const size_t testDataSize = (sizeof(testData) / sizeof(testData[0]));

static UA_DateTime testInsertDataSuccess[] = {
    50,
    250,
    550
};
static const size_t testInsertDataSuccessSize = (sizeof(testInsertDataSuccess) / sizeof(testInsertDataSuccess[0]));


static UA_DateTime testInsertResultData[] = {
    50,
    100,
    200,
    250,
    300,
    400,
    500,
    550
};
static size_t testInsertResultDataSize = (sizeof(testInsertResultData) / sizeof(testInsertResultData[0]));

static UA_DateTime testReplaceDataSuccess[] = {
    100,
    300,
    500
};
static const size_t testReplaceDataSuccessSize = (sizeof(testReplaceDataSuccess) / sizeof(testReplaceDataSuccess[0]));

struct DeleteRange {
    UA_DateTime start;
    UA_DateTime end;
    size_t historySize;
    UA_StatusCode statusCode;
};

static struct DeleteRange testDeleteRangeData[] = {
{200, 400, 3, UA_STATUSCODE_GOOD},
{100, 400, 2, UA_STATUSCODE_GOOD},
{200, 500, 2, UA_STATUSCODE_GOOD},
{100, 500, 1, UA_STATUSCODE_GOOD},
{100, 550, 0, UA_STATUSCODE_GOOD},
{50, 550, 0, UA_STATUSCODE_GOOD},
{500, 550, 4, UA_STATUSCODE_GOOD},
{50, 150, 4, UA_STATUSCODE_GOOD},
{100, 100, 4, UA_STATUSCODE_GOOD},
{500, 500, 4, UA_STATUSCODE_GOOD},
{200, 200, 4, UA_STATUSCODE_GOOD},
{50, 50, 5, UA_STATUSCODE_BADNODATA},
{550, 550, 5, UA_STATUSCODE_BADNODATA},
{150, 150, 5, UA_STATUSCODE_BADNODATA},
{200, 100, 5, UA_STATUSCODE_BADTIMESTAMPNOTSUPPORTED},
{LLONG_MIN, LLONG_MAX, 0, UA_STATUSCODE_GOOD},
{0, LLONG_MAX, 0, UA_STATUSCODE_GOOD},
{LLONG_MIN, 0, 5, UA_STATUSCODE_BADNODATA},
{0, 0, 5, UA_STATUSCODE_BADNODATA},
{50, 75, 5, UA_STATUSCODE_BADNODATA},
{50, 100, 5, UA_STATUSCODE_BADNODATA},
{550, 600, 5, UA_STATUSCODE_BADNODATA}
};
static const size_t testDeleteRangeDataSize = (sizeof(testDeleteRangeData) / sizeof(testDeleteRangeData[0]));

#endif /*UA_HISTORICAL_READ_TEST_DATA_H_*/
