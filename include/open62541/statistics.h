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

/**
 * Statistic counters
 * ------------------
 *
 * The stack manage statistic counter for the following layers:
 * - Network
 * - Secure channel
 * - Session
 *
 * The session layer counters are matching the counters of the
 * ServerDiagnosticsSummaryDataType that are defined in the OPC UA Part 5
 * specification. Counter of the other layers are not specified by OPC UA but
 * are harmonized with the session layer counters if possible.
 *
 * To get a snapshot of the counters in runtime, call the
 * UA_Server_getStatistics() or UA_Client_getStatistics() function. */

typedef enum {
   UA_DIAGNOSTICEVENT_CLOSE,
   UA_DIAGNOSTICEVENT_SECURITYREJECT,
   UA_DIAGNOSTICEVENT_REJECT,
   UA_DIAGNOSTICEVENT_TIMEOUT,
   UA_DIAGNOSTICEVENT_ABORT,
   UA_DIAGNOSTICEVENT_PURGE
} UA_DiagnosticEvent;

typedef struct {
    UA_UInt32 currentConnectionCount;
    UA_UInt32 cumulatedConnectionCount;
    UA_UInt32 rejectedConnectionCount;
    UA_UInt32 connectionTimeoutCount;
    UA_UInt32 connectionAbortCount;
} UA_NetworkStatistics;

typedef struct {
    UA_UInt32 currentChannelCount;
    UA_UInt32 cumulatedChannelCount;
    UA_UInt32 rejectedChannelCount;
    UA_UInt32 channelTimeoutCount; /* only used by servers */
    UA_UInt32 channelAbortCount;
    UA_UInt32 channelPurgeCount; /* only used by servers */
} UA_SecureChannelStatistics;

typedef struct {
    UA_UInt32 currentSessionCount;
    UA_UInt32 cumulatedSessionCount;
    UA_UInt32 securityRejectedSessionCount; /* only used by servers */
    UA_UInt32 rejectedSessionCount;
    UA_UInt32 sessionTimeoutCount; /* only used by servers */
    UA_UInt32 sessionAbortCount; /* only used by servers */
} UA_SessionStatistics;

_UA_END_DECLS

#endif /* UA_STATISTICS_H_ */
