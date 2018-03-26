/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_H_
#define UA_PUBSUB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <queue.h>
#include "ua_plugin_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include "ua_server.h"
#include "ua_server_pubsub.h"

//forward declarations
struct UA_PubSubConnection;
typedef struct UA_PubSubConnection UA_PubSubConnection;

/**********************************************/
/*               Connection                   */
/**********************************************/
//the connection config (public part of connection) object is defined in include/ua_plugin_pubsub.h
struct UA_PubSubConnection{
    UA_PubSubConnectionConfig *config;
    //internal fields
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
};

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst);

void UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig);
void UA_PubSubConnection_delete(UA_PubSubConnection *connection);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PUBSUB_H_ */
