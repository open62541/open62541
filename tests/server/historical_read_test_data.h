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

typedef struct {
    UA_DateTime start;
    UA_DateTime end;
    UA_UInt32 numValuesPerNode;
    UA_Boolean returnBounds;
    UA_DateTime result[8];
    UA_Boolean returnContinuationPoint;
} testTuple;

#define TIMESTAMP_UNSPECIFIED LLONG_MIN
#define NODATA 0
#define TIMESTAMP_FIRST 1
#define TIMESTAMP_4_48 (448 * UA_DATETIME_SEC)
#define TIMESTAMP_4_58 (458 * UA_DATETIME_SEC)
#define TIMESTAMP_4_59 (459 * UA_DATETIME_SEC)
#define TIMESTAMP_5_00 (500 * UA_DATETIME_SEC)
#define TIMESTAMP_5_01 (501 * UA_DATETIME_SEC)
#define TIMESTAMP_5_02 (502 * UA_DATETIME_SEC)
#define TIMESTAMP_5_03 (503 * UA_DATETIME_SEC)
#define TIMESTAMP_5_04 (504 * UA_DATETIME_SEC)
#define TIMESTAMP_5_05 (505 * UA_DATETIME_SEC)
#define TIMESTAMP_5_06 (506 * UA_DATETIME_SEC)
#define TIMESTAMP_5_07 (507 * UA_DATETIME_SEC)
#define TIMESTAMP_LAST (600 * UA_DATETIME_SEC)

static UA_DateTime testData[] = {
    TIMESTAMP_5_03,
    TIMESTAMP_5_00,
    TIMESTAMP_5_02,
    TIMESTAMP_5_06,
    TIMESTAMP_5_05,
    0 // last element
};

#define DELETE_START_TIME TIMESTAMP_5_03
#define DELETE_STOP_TIME  TIMESTAMP_5_06

static UA_DateTime testDataAfterDelete[] = {
    TIMESTAMP_5_00,
    TIMESTAMP_5_02,
    TIMESTAMP_5_06,
    0 // last element
};

static UA_StatusCode testDataUpdateResult[] = {
    UA_STATUSCODE_GOODENTRYREPLACED,
    UA_STATUSCODE_GOODENTRYREPLACED,
    UA_STATUSCODE_GOODENTRYINSERTED,
    UA_STATUSCODE_GOODENTRYINSERTED,
    UA_STATUSCODE_GOODENTRYREPLACED
};

static testTuple testRequests[] =
{
    { TIMESTAMP_5_00,
      TIMESTAMP_5_05,
      0,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_05,
      0,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_04,
      0,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_04,
      0,
      false,
      { TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_05,
      TIMESTAMP_5_00,
      0,
      true,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_5_05,
      TIMESTAMP_5_00,
      0,
      false,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_5_04,
      TIMESTAMP_5_01,
      0,
      true,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_5_04,
      TIMESTAMP_5_01,
      0,
      false,
      { TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_4_59,
      TIMESTAMP_5_05,
      0,
      true,
      { TIMESTAMP_FIRST, TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, 0 },
      false
    },
    { TIMESTAMP_4_59,
      TIMESTAMP_5_05,
      0,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_07,
      0,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, TIMESTAMP_5_06, TIMESTAMP_LAST, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_07,
      0,
      false,
      { TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, TIMESTAMP_5_06, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_05,
      3,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_05,
      3,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_04,
      3,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_04,
      3,
      false,
      { TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_05,
      TIMESTAMP_5_00,
      3,
      true,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      true
    },
    { TIMESTAMP_5_05,
      TIMESTAMP_5_00,
      3,
      false,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_5_04,
      TIMESTAMP_5_01,
      3,
      true,
      { TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      true
    },
    { TIMESTAMP_5_04,
      TIMESTAMP_5_01,
      3,
      false,
      { TIMESTAMP_5_03, TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_4_59,
      TIMESTAMP_5_05,
      3,
      true,
      { TIMESTAMP_FIRST, TIMESTAMP_5_00, TIMESTAMP_5_02, 0 },
      true
    },
    { TIMESTAMP_4_59,
      TIMESTAMP_5_05,
      3,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_07,
      3,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_07,
      3,
      false,
      { TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, 0 },
      true
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_UNSPECIFIED,
      3,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_UNSPECIFIED,
      3,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_UNSPECIFIED,
      6,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, TIMESTAMP_5_06, TIMESTAMP_LAST, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_UNSPECIFIED,
      6,
      false,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, TIMESTAMP_5_03, TIMESTAMP_5_05, TIMESTAMP_5_06, 0 },
      false
    },
    { TIMESTAMP_5_07,
      TIMESTAMP_UNSPECIFIED,
      6,
      true,
      { TIMESTAMP_5_06, TIMESTAMP_LAST, 0 },
      false
    },
    { TIMESTAMP_5_07,
      TIMESTAMP_UNSPECIFIED,
      6,
      false,
      { NODATA, 0 },
      false
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_5_06,
      3,
      true,
      { TIMESTAMP_5_06,TIMESTAMP_5_05,TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_5_06,
      3,
      false,
      { TIMESTAMP_5_06,TIMESTAMP_5_05,TIMESTAMP_5_03, 0 },
      true
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_5_06,
      6,
      true,
      { TIMESTAMP_5_06,TIMESTAMP_5_05,TIMESTAMP_5_03,TIMESTAMP_5_02,TIMESTAMP_5_00,TIMESTAMP_FIRST, 0 },
      false
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_5_06,
      6,
      false,
      { TIMESTAMP_5_06, TIMESTAMP_5_05, TIMESTAMP_5_03, TIMESTAMP_5_02, TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_4_48,
      6,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_FIRST, 0 },
      false
    },
    { TIMESTAMP_UNSPECIFIED,
      TIMESTAMP_4_48,
      6,
      false,
      { NODATA, 0 },
      false
    },
    { TIMESTAMP_4_48,
      TIMESTAMP_4_48,
      0,
      true,
      { TIMESTAMP_FIRST, TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_4_48,
      TIMESTAMP_4_48,
      0,
      false,
      { NODATA, 0 },
      false
    },
    { TIMESTAMP_4_48,
      TIMESTAMP_4_48,
      1,
      true,
      { TIMESTAMP_FIRST, 0 },
      true
    },
    { TIMESTAMP_4_48,
      TIMESTAMP_4_48,
      1,
      false,
      { NODATA, 0 },
      false
    },
    { TIMESTAMP_4_48,
      TIMESTAMP_4_48,
      2,
      true,
      { TIMESTAMP_FIRST,TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_00,
      0,
      true,
      { TIMESTAMP_5_00,TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_00,
      0,
      false,
      { TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_00,
      1,
      true,
      { TIMESTAMP_5_00, 0 },
      true
    },
    { TIMESTAMP_5_00,
      TIMESTAMP_5_00,
      1,
      false,
      { TIMESTAMP_5_00, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_01,
      0,
      true,
      { TIMESTAMP_5_00, TIMESTAMP_5_02, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_01,
      0,
      false,
      { NODATA, 0 },
      false
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_01,
      1,
      true,
      { TIMESTAMP_5_00, 0 },
      true
    },
    { TIMESTAMP_5_01,
      TIMESTAMP_5_01,
      1,
      false,
      { NODATA },
      false
    },
    {0,0,0,false,{ NODATA }, false} // last element
};
#endif /*UA_HISTORICAL_READ_TEST_DATA_H_*/
