/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_connection.h"
#include "ua_log.h"

/** @defgroup server Server */

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

struct UA_Server;
typedef struct UA_Server UA_Server;

UA_Server UA_EXPORT * UA_Server_new(UA_String *endpointUrl, UA_ByteString *serverCertificate);
void UA_EXPORT UA_Server_delete(UA_Server *server);
void UA_EXPORT UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

/* Services for local use */
UA_AddNodesResult UA_EXPORT UA_Server_addNode(UA_Server *server, UA_Node **node, const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId);
void UA_EXPORT UA_Server_addReference(UA_Server *server, const UA_AddReferencesRequest *request, UA_AddReferencesResponse *response);
UA_AddNodesResult UA_EXPORT UA_Server_addScalarVariableNode(UA_Server *server, UA_String *browseName, void *value,
                                                            const UA_VTable_Entry *vt, const UA_NodeId *parentNodeId,
                                                            const UA_NodeId *referenceTypeId );

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
