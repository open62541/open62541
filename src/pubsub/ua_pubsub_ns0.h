/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef OPEN62541_UA_PUBSUB_NS0_H
#define OPEN62541_UA_PUBSUB_NS0_H

#ifdef __cplusplus
extern "C" {
#endif

#include "server/ua_server_internal.h"
#include "ua_pubsub.h"

UA_StatusCode
UA_Server_initPubSubNS0(UA_Server *server);

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);
UA_StatusCode
removePubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);
UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);
UA_StatusCode
removeWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);
UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);
UA_StatusCode
removeDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);
UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);
UA_StatusCode
removePublishedDataSetRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //OPEN62541_UA_PUBSUB_NS0_H
