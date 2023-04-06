/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_PUBSUB_NS0_H_
#define UA_PUBSUB_NS0_H_

#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

UA_StatusCode
initPubSubNS0(UA_Server *server);

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection);

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup);

UA_StatusCode
addReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup);

UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter);

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet);

UA_StatusCode
addStandaloneSubscribedDataSetRepresentation(UA_Server *server, UA_StandaloneSubscribedDataSet *subscribedDataSet);

UA_StatusCode
addDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader *dataSetReader);

UA_StatusCode
connectDataSetReaderToDataSet(UA_Server *server, UA_NodeId dsrId, UA_NodeId standaloneSdsId);

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
addSecurityGroupRepresentation(UA_Server *server, UA_SecurityGroup *securityGroup);
#endif /* UA_ENABLE_PUBSUB_SKS */

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */

_UA_END_DECLS

#endif /* UA_PUBSUB_NS0_H_ */
