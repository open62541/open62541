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

/* The configuration structs (public part of PubSub entities) are defined in include/ua_plugin_pubsub.h */

/**********************************************/
/*            PublishedDataSet                */
/**********************************************/
typedef struct{
    UA_PublishedDataSetConfig config;
    UA_DataSetMetaDataType dataSetMetaData;
    LIST_HEAD(UA_ListOfPubSubDataSetField, UA_PubSubDataSetField) fields;
    UA_NodeId identifier;
    UA_UInt16 fieldSize;
    UA_UInt16 promotedFieldsCount;
} UA_PublishedDataSet;

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src, UA_PublishedDataSetConfig *dst);
UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier);
void
UA_PublishedDataSet_delete(UA_PublishedDataSet *publishedDataSet);

/**********************************************/
/*               Connection                   */
/**********************************************/
//the connection config (public part of connection) object is defined in include/ua_plugin_pubsub.h
typedef struct{
    UA_PubSubConnectionConfig *config;
    //internal fields
    UA_PubSubChannel *channel;
    UA_NodeId identifier;
} UA_PubSubConnection;

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst);
UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier);
void
UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig);
void
UA_PubSubConnection_delete(UA_PubSubConnection *connection);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PUBSUB_H_ */
