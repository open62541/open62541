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

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util_internal.h"
#include "ua_server.h"
#include "ua_securechannel.h"
#include "../../deps/queue.h"

typedef struct channel_entry {
    UA_SecureChannel channel;
    TAILQ_ENTRY(channel_entry) pointers;
} channel_entry;

typedef struct {
    TAILQ_HEAD(, channel_entry) channels; // doubly-linked list of channels
    UA_UInt32 currentChannelCount;
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
UA_SecureChannelManager_create(UA_SecureChannelManager *const cm, UA_Connection *const connection,
                               const UA_SecurityPolicy *const securityPolicy,
                               const UA_AsymmetricAlgorithmSecurityHeader *const asymHeader);

UA_StatusCode
UA_SecureChannelManager_open(UA_SecureChannelManager *cm, UA_SecureChannel *channel,
                             const UA_OpenSecureChannelRequest *request,
                             UA_OpenSecureChannelResponse *response);

UA_StatusCode
UA_SecureChannelManager_renew(UA_SecureChannelManager *cm, UA_SecureChannel *channel,
                              const UA_OpenSecureChannelRequest *request,
                              UA_OpenSecureChannelResponse *response);

UA_SecureChannel *
UA_SecureChannelManager_get(UA_SecureChannelManager *cm, UA_UInt32 channelId);

UA_StatusCode
UA_SecureChannelManager_close(UA_SecureChannelManager *cm, UA_UInt32 channelId);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CHANNEL_MANAGER_H_ */
