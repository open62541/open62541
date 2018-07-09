/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_SESSION_MANAGER_H_
#define UA_SESSION_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_util_internal.h"
#include "ua_session.h"
#include "../../deps/queue.h"

typedef struct session_list_entry {
    LIST_ENTRY(session_list_entry) pointers;
    UA_Session session;
} session_list_entry;

typedef struct UA_SessionManager {
    LIST_HEAD(session_list, session_list_entry) sessions; // doubly-linked list of sessions
    UA_UInt32 currentSessionCount;
    UA_Server *server;
} UA_SessionManager;

UA_StatusCode
UA_SessionManager_init(UA_SessionManager *sm, UA_Server *server);

/* Deletes all sessions */
void UA_SessionManager_deleteMembers(UA_SessionManager *sm);

/* Deletes all sessions that have timed out. Deletion is implemented via a
 * delayed callback. So all currently scheduled jobs with a pointer to the
 * session can complete. */
void UA_SessionManager_cleanupTimedOut(UA_SessionManager *sm,
                                       UA_DateTime nowMonotonic);

UA_StatusCode
UA_SessionManager_createSession(UA_SessionManager *sm, UA_SecureChannel *channel,
                                const UA_CreateSessionRequest *request, UA_Session **session);

UA_StatusCode
UA_SessionManager_removeSession(UA_SessionManager *sm, const UA_NodeId *token);

UA_Session *
UA_SessionManager_getSessionByToken(UA_SessionManager *sm, const UA_NodeId *token);

UA_Session *
UA_SessionManager_getSessionById(UA_SessionManager *sm, const UA_NodeId *sessionId);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SESSION_MANAGER_H_ */
