/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/client_highlevel.h>
#include "ua_client_internal.h"
#include "ziptree.h"

typedef struct NodeIdTreeEntry {
    ZIP_ENTRY(NodeIdTreeEntry) zipfields;
    UA_NodeId nodeId;
    UA_String typeName;
} NodeIdTreeEntry;

static enum ZIP_CMP
cmpNodeIdTreeEntry(const void *a, const void *b) {
    const UA_NodeId *aa = (const UA_NodeId*)a;
    const UA_NodeId *bb = (const UA_NodeId*)b;
    return (enum ZIP_CMP)UA_NodeId_order(aa, bb);
}

typedef ZIP_HEAD(NodeIdTree, NodeIdTreeEntry) NodeIdTree;
ZIP_FUNCTIONS(NodeIdTree, NodeIdTreeEntry, zipfields,
              UA_NodeId, nodeId, cmpNodeIdTreeEntry)

static void *
deleteNodeIdEntry(void *context, NodeIdTreeEntry *elm) {
    UA_NodeId_clear(&elm->nodeId);
    UA_String_clear(&elm->typeName);
    UA_free(elm);
    return NULL;
}

static UA_StatusCode
browseDataTypesRecursive(UA_Client *client, NodeIdTree *tree,
                         size_t *treeSize, UA_NodeId typeNode) {
    /* Set up the browse request */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = typeNode;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
    bd.nodeClassMask = UA_NODECLASS_DATATYPE;
    bd.resultMask = UA_BROWSERESULTMASK_DISPLAYNAME; // XX

    /* Browse */
    UA_BrowseResult br = UA_Client_browse(client, NULL, 0, &bd);
    UA_StatusCode res = br.statusCode;
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Could not browse the DataTypeNodes with status code %s",
                     UA_StatusCode_name(res));
        UA_BrowseResult_clear(&br);
        return res;
    }

    /* Check which types are unknown, add them and recurse */
    for(size_t i = 0; i < br.referencesSize && res == UA_STATUSCODE_GOOD; i++) {
        UA_ReferenceDescription *rd = &br.references[i];
        if(!UA_ExpandedNodeId_isLocal(&rd->nodeId))
            continue;

        /* Skip known types */
        if(UA_findDataTypeWithCustom(&rd->nodeId.nodeId,
                                     client->config.customDataTypes))
            continue;
        if(ZIP_FIND(NodeIdTree, tree, &rd->nodeId.nodeId))
            continue;

        /* Create an entry */
        NodeIdTreeEntry *entry = (NodeIdTreeEntry*)UA_malloc(sizeof(NodeIdTreeEntry));
        if(!entry) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            break;
        }
        *entry = (NodeIdTreeEntry){{NULL, NULL}, rd->nodeId.nodeId, rd->displayName.text};
            
        ZIP_INSERT(NodeIdTree, tree, entry);
        (*treeSize)++;

        /* Recurse */
        res = browseDataTypesRecursive(client, tree, treeSize, rd->nodeId.nodeId);

        /* Don't double-free */
        UA_NodeId_init(&rd->nodeId.nodeId);
        UA_String_init(&rd->displayName.text);
    }

    UA_BrowseResult_clear(&br);
    return res;
}

static void *
setNodesToRead(void *context, NodeIdTreeEntry *elm) {
    UA_ReadValueId **rvi = (UA_ReadValueId**)context;
    (*rvi)->nodeId = elm->nodeId;
    (*rvi)->attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    (*rvi)++;
    return NULL;
}

UA_StatusCode
UA_Client_getRemoteDataTypes(UA_Client *client,
                             UA_DataTypeArray *customTypes) {
    if(customTypes->typesSize != 0 || customTypes->types != NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    size_t treeSize = 0;
    NodeIdTree tree;
    ZIP_INIT(&tree);

    /* Get all unknown DataType NodeIds */
    UA_StatusCode res = browseDataTypesRecursive(client, &tree, &treeSize,
                                                 UA_NS0ID(STRUCTURE));
    if(res != UA_STATUSCODE_GOOD) {
        ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
        return res;
    }

    if(treeSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Allocate the array of custom types */
    UA_DataType *types = (UA_DataType*)UA_calloc(treeSize, sizeof(UA_DataType));
    if(!types) {
        ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Read the descriptions of the unknown types */
    UA_ReadValueId *nodesToRead = (UA_ReadValueId*)
        UA_calloc(treeSize, sizeof(UA_ReadValueId));
    if(!nodesToRead) {
        UA_free(types);
        ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_ReadValueId *tmp = nodesToRead;
    ZIP_ITER(NodeIdTree, &tree, setNodesToRead, &tmp);

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = nodesToRead;
    req.nodesToReadSize = treeSize;

    UA_ReadResponse rr = UA_Client_Service_read(client, req);
    res = rr.responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD && rr.resultsSize != treeSize)
        res = UA_STATUSCODE_BADINTERNALERROR;
    if(res != UA_STATUSCODE_GOOD) {
        ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
        UA_free(types);
        UA_ReadRequest_clear(&req);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Create the new types */
    size_t typesSize = 0;
    for(size_t i = 0; i < treeSize; i++) {
        UA_DataValue *dv = &rr.results[i];
        if(dv->hasStatus && dv->status != UA_STATUSCODE_GOOD)
            continue;
        if(!dv->hasValue || dv->value.type != &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])
            continue;
        NodeIdTreeEntry *entry = ZIP_FIND(NodeIdTree, &tree, &nodesToRead[i].nodeId);
        UA_StructureDefinition *sd = (UA_StructureDefinition*)dv->value.data;
        res = UA_DataType_fromStructureDefinition(&types[typesSize], sd,
                                                  nodesToRead[i].nodeId, entry->typeName,
                                                  client->config.customDataTypes);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Could not add the DataType %S (%N) with status code %s",
                         entry->typeName, nodesToRead[i].nodeId, UA_StatusCode_name(res));
            continue;
        }
        typesSize++;
    }

    /* Clean up */
    ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
    UA_ReadRequest_clear(&req);

    /* Set the output types array */
    if(typesSize > 0) {
        customTypes->types = types;
        customTypes->typesSize = typesSize;
        customTypes->cleanup = true; /* Contains allocated data that needs to be freed */
    } else {
        UA_free(types);
    }
    return UA_STATUSCODE_GOOD;
}
