/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef UA_DRIVER_ALARMS_CONDITIONS_H_
#define UA_DRIVER_ALARMS_CONDITIONS_H_

#include <open62541/server.h>

/**
 * Alarms & Conditions Driver
 * --------------------------
 * Provides the server-side bookkeeping for OPC UA Alarms & Conditions. The
 * public A&C server API in server.h operates on an instance of this driver
 * attached to the server via UA_Server_addDriver. */

UA_EXPORT UA_Driver *
UA_AlarmsConditionsDriver_default(const UA_KeyValueMap params);

#endif /* UA_DRIVER_ALARMS_CONDITIONS_H_ */
