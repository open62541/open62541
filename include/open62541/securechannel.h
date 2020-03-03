/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Mark Giraud)
 */

#ifndef OPEN62541_SECURECHANNEL_H_
#define OPEN62541_SECURECHANNEL_H_

#include <open62541/nodeids.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

_UA_BEGIN_DECLS

typedef struct {
    UA_UInt32 protocolVersion;
    UA_UInt32 localMaxMessageSize;  /* (0 = unbounded) */
    UA_UInt32 remoteMaxMessageSize; /* (0 = unbounded) */
    UA_UInt32 localMaxChunkCount;   /* (0 = unbounded) */
    UA_UInt32 remoteMaxChunkCount;  /* (0 = unbounded) */
} UA_SecureChannelConfig;

_UA_END_DECLS

#endif //OPEN62541_SECURECHANNEL_H_
