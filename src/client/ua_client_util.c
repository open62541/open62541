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

static UA_StatusCode
getRemoteDataTypes(UA_Client *client, UA_ReadRequest *req,
                   UA_DataTypeArray **outCustomTypes) {
    UA_ReadResponse rr = UA_Client_Service_read(client, *req);
    UA_StatusCode res = rr.responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD && rr.resultsSize != req->nodesToReadSize)
        res = UA_STATUSCODE_BADINTERNALERROR;
    if(res != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_clear(&rr);
        return res;
    }

    /* Allocate the arrays */
    size_t typesSize = req->nodesToReadSize / 2;
    UA_DataTypeArray *dta = (UA_DataTypeArray*)
        UA_calloc(1, sizeof(UA_DataTypeArray));
    if(!dta) {
        UA_ReadResponse_clear(&rr);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    dta->cleanup = true;
    dta->types = (UA_DataType*)UA_calloc(typesSize, sizeof(UA_DataType));
    if(!dta->types) {
        UA_ReadResponse_clear(&rr);
        UA_cleanupDataTypeWithCustom(dta);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Create the new types */
    for(size_t i = 0; i < typesSize; i++) {
        UA_DataValue *typeDef = &rr.results[i];
        if(typeDef->hasStatus && typeDef->status != UA_STATUSCODE_GOOD)
            continue;
        if(!typeDef->hasValue || !UA_Variant_hasScalarType(&typeDef->value,
                                                           &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]))
            continue;
        UA_StructureDefinition *sd = (UA_StructureDefinition*)typeDef->value.data;

        UA_DataValue *typeName = &rr.results[i+typesSize];
        if(typeName->hasStatus && typeName->status != UA_STATUSCODE_GOOD)
            continue;
        if(!typeName->hasValue || !UA_Variant_hasScalarType(&typeName->value,
                                                            &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]))
            continue;
        UA_LocalizedText *name = (UA_LocalizedText*)typeName->value.data;

        res = UA_DataType_fromStructureDefinition(&dta->types[dta->typesSize], sd,
                                                  req->nodesToRead[i].nodeId, name->text,
                                                  client->config.customDataTypes);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Could not add the DataType %S (%N) with status code %s",
                         name->text, req->nodesToRead[i].nodeId, UA_StatusCode_name(res));
            continue;
        }

        UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                    "Added DataType %S (%N) to the DataTypeArray",
                    name->text, req->nodesToRead[i].nodeId);
        dta->typesSize++;
    }

    *outCustomTypes = dta;

    UA_ReadRequest_clear(req);
    if(dta->typesSize == 0) {
        UA_free(dta->types);
        dta->types = NULL;
    }
    return UA_STATUSCODE_GOOD;
}

static void *
setNodesToRead(void *context, NodeIdTreeEntry *elm) {
    UA_ReadValueId **rvi = (UA_ReadValueId**)context;
    (*rvi)->nodeId = elm->nodeId;
    (*rvi)->attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    (*rvi)++;
    UA_free(elm);
    return NULL;
}

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_getRemoteDataTypes(UA_Client *client,
                             size_t dataTypesNodesSize,
                             const UA_NodeId *dataTypesNodes,
                             UA_DataTypeArray **customTypes) {
    if(!customTypes)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);

    /* req.nodesToRead has twice the normal length. Because we read the
     * DataTypeDescription and the DisplayName in the same request. */

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(dataTypesNodesSize == 0) {
        /* Browse to find unknown datatypes in the information model */

        /* Get all unknown DataType NodeIds */
        NodeIdTree tree;
        ZIP_INIT(&tree);
        size_t treeSize = 0;
        res = browseDataTypesRecursive(client, &tree, &treeSize, UA_NS0ID(STRUCTURE));
        if(res != UA_STATUSCODE_GOOD) {
            ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
            return res;
        }

        if(treeSize == 0)
            return UA_STATUSCODE_GOOD;

        /* Read the descriptions of the unknown types */
        UA_ReadValueId *nodesToRead = (UA_ReadValueId*)
            UA_calloc(treeSize * 2, sizeof(UA_ReadValueId));
        if(!nodesToRead) {
            ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        UA_ReadValueId *tmp = nodesToRead;
        ZIP_ITER(NodeIdTree, &tree, setNodesToRead, &tmp);
        req.nodesToRead = nodesToRead;
        req.nodesToReadSize = treeSize;
    } else {
        /* Prepare the ReadRequest for the provided NodeIds */
        req.nodesToRead = (UA_ReadValueId*)
            UA_Array_new(dataTypesNodesSize * 2, &UA_TYPES[UA_TYPES_READVALUEID]);
        if(!req.nodesToRead)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        req.nodesToReadSize = dataTypesNodesSize;

        for(size_t i = 0; i < dataTypesNodesSize; i++) {
            req.nodesToRead[i].attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
            res |= UA_NodeId_copy(&dataTypesNodes[i], &req.nodesToRead[i].nodeId);
        }

        if(res != UA_STATUSCODE_GOOD) {
            UA_ReadRequest_clear(&req);
            return res;
        }
    }

    /* Prepare reading the Displayname attributes additionally */
    for(size_t i = 0; i < req.nodesToReadSize; i++) {
        res |= UA_NodeId_copy(&req.nodesToRead[i].nodeId,
                              &req.nodesToRead[i + req.nodesToReadSize].nodeId);
        req.nodesToRead[i + req.nodesToReadSize].attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    }
    req.nodesToReadSize *= 2;
    if(res != UA_STATUSCODE_GOOD) {
        UA_ReadRequest_clear(&req);
        return res;
    }

    /* Read and convert the DataTypeDefinitions */
    res = getRemoteDataTypes(client, &req, customTypes);
    UA_ReadRequest_clear(&req);
    return res;
}
