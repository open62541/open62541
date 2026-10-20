/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_LOGOBJECT_INTERNAL_H_
#define UA_LOGOBJECT_INTERNAL_H_

#include <open62541/driver/logobject.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#define UA_DRIVER_LOGOBJECT_NAME "logobject"

typedef struct LogObjectDriver {
    UA_Driver driver; /* Must be the first member */
    UA_LogObjectBackend backend;
} LogObjectDriver;

/* Identify the driver before casting a generic UA_Driver */
UA_Boolean
isLogObjectDriver(const UA_Driver *driver);

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_LOGOBJECT_INTERNAL_H_ */
