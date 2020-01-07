/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_STATISTICS_H_
#define UA_STATISTICS_H_

#include <open62541/types.h>

_UA_BEGIN_DECLS

typedef struct {
    UA_UInt32 currentConnections;
    UA_UInt32 closedConnections;
    UA_UInt32 timedoutConnections;
} UA_NetworkStatistics;

typedef struct {
    UA_UInt32 currentChannels;
    UA_UInt32 closedChannels;
    UA_UInt32 timedoutChannels;
    UA_UInt32 purgedChannels;
    UA_UInt32 outOfChannels; /* only used by servers */
}UA_SecureChannelStatistics;

typedef struct {
    UA_UInt32 currentSessions;
    UA_UInt32 closedSessions;
    UA_UInt32 timedoutSessions; /* only used by servers */
    UA_UInt32 outOfSessions; /* only used by servers */
    UA_UInt32 activateSessionFailures;
    UA_UInt32 createSessionFailures;
}UA_SessionStatistics;

_UA_END_DECLS

#endif /* UA_STATISTICS_H_ */
