/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_NS0_H_
#define UA_PUBSUB_NS0_H_

#include "server/ua_server_internal.h"
#include "ua_pubsub.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

UA_StatusCode
UA_Server_initPubSubNS0(UA_Server *server);

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);

UA_StatusCode
removePubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);

UA_StatusCode
removeGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);

UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);

UA_StatusCode
removeDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

UA_StatusCode
removePublishedDataSetRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */

_UA_END_DECLS

#endif /* UA_PUBSUB_NS0_H_ */
