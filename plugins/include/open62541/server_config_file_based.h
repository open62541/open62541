/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_SERVER_CONFIG_FILE_BASED_H
#define UA_SERVER_CONFIG_FILE_BASED_H

#include <open62541/server.h>
#include <stdio.h>
#include <errno.h>

_UA_BEGIN_DECLS

/* Loads the server configuration from a Json5 file into the server.
 *
 * @param json The configuration in json5 format.
 */
UA_EXPORT UA_Server *
UA_Server_newFromFile(const UA_ByteString json_config);

/* Loads the server configuration from a Json5 file into the server.
 *
 * @param config The server configuration.
 * @param json The configuration in json5 format.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_updateFromFile(UA_ServerConfig *config, const UA_ByteString json_config);

_UA_END_DECLS

#endif //UA_SERVER_CONFIG_FILE_BASED_H
