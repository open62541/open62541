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

struct UA_SecureChannelManager;
typedef struct UA_SecureChannelManager UA_SecureChannelManager;

struct UA_SessionManager;
typedef struct UA_SessionManager UA_SessionManager;

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

typedef struct UA_Server {
    UA_ApplicationDescription description;
    UA_SecureChannelManager *secureChannelManager;
    UA_SessionManager *sessionManager;
    UA_NodeStore *nodestore;
    UA_Logger logger;
    UA_ByteString serverCertificate;

    // todo: move these somewhere sane
    UA_ExpandedNodeId objectsNodeId;
    UA_NodeId hasComponentReferenceTypeId;
} UA_Server;

void UA_LIBEXPORT UA_Server_init(UA_Server *server, UA_String *endpointUrl);
UA_Int32 UA_LIBEXPORT UA_Server_deleteMembers(UA_Server *server);
UA_Int32 UA_LIBEXPORT UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

/* Services for local use */
UA_AddNodesResult UA_Server_addScalarVariableNode(UA_Server *server, UA_String *browseName, void *value,
                                                  const UA_VTable_Entry *vt, UA_ExpandedNodeId *parentNodeId,
                                                  UA_NodeId *referenceTypeId );
UA_AddNodesResult UA_Server_addNode(UA_Server *server, UA_Node **node, UA_ExpandedNodeId *parentNodeId,
                                    UA_NodeId *referenceTypeId);
void UA_Server_addReferences(UA_Server *server, const UA_AddReferencesRequest *request,
                             UA_AddReferencesResponse *response);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
