/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_COMMON_H_
#define UA_EVENTLOOP_COMMON_H_

#include <open62541/plugin/eventloop.h>

/* Utility definitions to be used in EventLoop implementations.
 * Not part of the public API. */

_UA_BEGIN_DECLS

/* Typing restrictions for key-value parameters */
typedef struct {
    UA_QualifiedName name;
    const UA_DataType *type;
    UA_Boolean required;
    UA_Boolean scalar;
    UA_Boolean array;
} UA_KeyValueRestriction;

UA_StatusCode
UA_KeyValueRestriction_validate(const UA_Logger *logger,
                                const char *logprefix,
                                const UA_KeyValueRestriction *restrictions,
                                size_t restrictionsSize,
                                const UA_KeyValueMap *map);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_COMMON_H_ */
