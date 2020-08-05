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

#include <open62541/types_generated.h>  /* Defines UA data types */
#include <open62541/server.h>           /* Defines UA_Server     */

/* UA_PubSubManager_loadPubSubConfigFromByteString() */
/**
 * @brief       Decodes the information from the ByteString. If the decoded content is a PubSubConfiguration in a UABinaryFileDataType-object               
 *              it will overwrite the current PubSub configuration from the server.
 * 
 * @param       server      [bi]    Pointer to Server object that shall be configured
 * @param       buffer      [in]    Relative path and name of the file that contains the PubSub configuration
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_PubSubManager_loadPubSubConfigFromByteString
(
    /*[bi]*/    UA_Server *server, 
    /*[in]*/    const UA_ByteString buffer
);

/* UA_PubSubManager_getEncodedPubSubConfiguration() */
/**
 * @brief       Saves the current PubSub configuration of a server in a byteString.
 * 
 * @param       server  [in]    Pointer to server object, that contains the PubSubConfiguration
 * @param       buffer  [out]    Pointer to a byteString object
 *
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PubSubManager_getEncodedPubSubConfiguration
(
    /*[bi]*/    UA_Server *server, 
    /*[out]*/   UA_ByteString *buffer
);

#endif /* UA_PUBSUB_CONFIG_H_ */

#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */
