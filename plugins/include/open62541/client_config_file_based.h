/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef UA_CLIENT_CONFIG_FILE_BASED_H
#define UA_CLIENT_CONFIG_FILE_BASED_H

#include <open62541/client.h>

_UA_BEGIN_DECLS

/* Create a new client from a file.  The client configuration is loaded from a
 * Json5 file.
 *
 * @param json The configuration in json5 format.
 */
UA_EXPORT UA_Client *
UA_Client_newFromFile(const UA_ByteString json_config);

/* Update a client configuration from a file.  The client configuration is
 * loaded from a Json5 file into the server.
 *
 * @param config The client configuration.
 * @param json The configuration in json5 format.
 */
UA_EXPORT UA_StatusCode
UA_ClientConfig_updateFromFile(UA_ClientConfig *config, const UA_ByteString json_config);

_UA_END_DECLS

#endif //UA_CLIENT_CONFIG_FILE_BASED_H
