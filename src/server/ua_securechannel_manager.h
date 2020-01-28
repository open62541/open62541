/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_CHANNEL_MANAGER_H_
#define UA_CHANNEL_MANAGER_H_

#include <open62541/server.h>

#include "open62541_queue.h"
#include "ua_securechannel.h"
#include "ua_util_internal.h"
#include "ua_workqueue.h"

_UA_BEGIN_DECLS

typedef enum {
    UA_DIAGNOSTICEVENT_CLOSE,
    UA_DIAGNOSTICEVENT_REJECT,
    UA_DIAGNOSTICEVENT_SECURITYREJECT,
    UA_DIAGNOSTICEVENT_TIMEOUT,
    UA_DIAGNOSTICEVENT_ABORT,
    UA_DIAGNOSTICEVENT_PURGE
} UA_DiagnosticEvent;

typedef struct channel_entry {
    UA_DelayedCallback cleanupCallback;
    TAILQ_ENTRY(channel_entry) pointers;
    UA_SecureChannel channel;
} channel_entry;

typedef struct {
    TAILQ_HEAD(, channel_entry) channels;
    UA_UInt32 lastChannelId;
    UA_UInt32 lastTokenId;
    UA_Server *server;
} UA_SecureChannelManager;

UA_StatusCode
UA_SecureChannelManager_init(UA_SecureChannelManager *cm, UA_Server *server);

/* Remove a all securechannels */
void
UA_SecureChannelManager_deleteMembers(UA_SecureChannelManager *cm);

/* Remove timed out securechannels with a delayed callback. So all currently
 * scheduled jobs with a pointer to a securechannel can finish first. */
void
UA_SecureChannelManager_cleanupTimedOut(UA_SecureChannelManager *cm,
                                        UA_DateTime nowMonotonic);

UA_StatusCode
UA_SecureChannelManager_create(UA_SecureChannelManager *cm, UA_Connection *connection);

UA_StatusCode
UA_SecureChannelManager_config(UA_SecureChannelManager *cm, UA_SecureChannel *channel,
                               const UA_AsymmetricAlgorithmSecurityHeader *asymHeader);

UA_StatusCode
UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId,
                              UA_DiagnosticEvent event);

_UA_END_DECLS

#endif /* UA_CHANNEL_MANAGER_H_ */
