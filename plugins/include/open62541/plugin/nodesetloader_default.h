/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#ifndef UA_NODESET_LOADER_DEFAULT_H_
#define UA_NODESET_LOADER_DEFAULT_H_

#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

/* Initializing nodeset loader. */
UA_EXPORT void
UA_NodesetLoader_Init(UA_Server *server);

/* Load the typemodel at runtime, without the need to statically compile the model.
 * This is an alternative to the Python nodeset compiler approach. */
UA_EXPORT UA_StatusCode
UA_NodesetLoader_LoadNodeset(UA_Server *server, const char *nodeset2XmlFilePath);

/* Deinitializing nodeset loader. Deleting dynamicallly allocated data used
 * by the nodeset loader. */
UA_EXPORT void
UA_NodesetLoader_Delete(UA_Server *server);

_UA_END_DECLS

#endif /* UA_NODESET_LOADER_DEFAULT_H_ */
