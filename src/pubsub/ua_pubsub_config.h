/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 */

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

#ifndef UA_PUBSUB_CONFIG_H_
#define UA_PUBSUB_CONFIG_H_

#include <open62541/types_generated.h>
#include <open62541/server.h>

/* Decodes the information from the ByteString. If the decoded content is a
 * PubSubConfiguration in a UABinaryFileDataType-object. It will overwrite the
 * current PubSub configuration from the server. */
UA_StatusCode
UA_PubSubManager_loadPubSubConfigFromByteString(UA_Server *server,
                                                const UA_ByteString buffer);

/* Saves the current PubSub configuration of a server in a byteString. */
UA_StatusCode
UA_PubSubManager_getEncodedPubSubConfiguration(UA_Server *server,
                                               UA_ByteString *buffer);

#endif /* UA_PUBSUB_CONFIG_H_ */

#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */
