/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) SICK AG (Author: Joerg Fischer)
 */

#include <open62541/client_highlevel.h>
#include "ua_client_internal.h"
#include "ziptree.h"
#include <string.h>

typedef struct NodeIdTreeEntry {
    ZIP_ENTRY(NodeIdTreeEntry) zipfields;
    UA_NodeId nodeId;
    UA_String typeName;
    UA_NodeId builtinHint; /* resolved builtin supertype (namespace 0) if found */
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
    UA_NodeId_clear(&elm->builtinHint);
    UA_free(elm);
    return NULL;
}

/* Follow the HasSubtype chain (inverse) to find a namespace-0 builtin supertype. */
static UA_StatusCode
resolveBuiltinHint(UA_Client *client, const UA_NodeId *start, UA_NodeId *outBuiltin) {
    UA_NodeId_init(outBuiltin);

    UA_NodeId current;
    UA_StatusCode retval = UA_NodeId_copy(start, &current);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;


    while(true) {
        UA_BrowseDescription bd;
        UA_BrowseDescription_init(&bd);
        bd.nodeId = current;
        bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
        bd.referenceTypeId = UA_NS0ID(HASSUBTYPE);
        bd.nodeClassMask = UA_NODECLASS_DATATYPE;

        UA_BrowseResult br = UA_Client_browse(client, NULL, 0, &bd);
        UA_StatusCode res = br.statusCode;
        if(res != UA_STATUSCODE_GOOD) {
            UA_BrowseResult_clear(&br);
            UA_NodeId_clear(&current);
            return res;
        }

        bool moved = false;
        for(size_t i = 0; i < br.referencesSize; i++) {
            UA_ReferenceDescription *rd = &br.references[i];
            if(!UA_ExpandedNodeId_isLocal(&rd->nodeId))
                continue;

            /* If the supertype is in namespace 0, record it as builtin hint. */
            if(rd->nodeId.nodeId.namespaceIndex == 0) {
                UA_NodeId_copy(&rd->nodeId.nodeId, outBuiltin);
                moved = false;
                break;
            }

            /* Otherwise follow the supertype up one level and continue. */
            UA_NodeId_clear(&current);
            UA_NodeId_copy(&rd->nodeId.nodeId, &current);
            moved = true;
            break;
        }

        UA_BrowseResult_clear(&br);
        if(!moved)
            break;
    }

    UA_NodeId_clear(&current);
    return UA_STATUSCODE_GOOD;
}

static void *
populateBuiltinHint(void *context, NodeIdTreeEntry *elm) {
    UA_Client *client = (UA_Client*)context;
    UA_StatusCode res = resolveBuiltinHint(client, &elm->nodeId, &elm->builtinHint);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Could not resolve builtin hint for %N: %s",
                     elm->nodeId, UA_StatusCode_name(res));
    } else if(!UA_NodeId_isNull(&elm->builtinHint) &&
              elm->builtinHint.namespaceIndex == 0) {
        UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Resolved builtin hint for %N -> %N",
                     elm->nodeId, elm->builtinHint);
    } else {
        UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "No builtin hint for %N",
                     elm->nodeId);
    }
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

        const UA_Boolean isKnown =
            UA_findDataTypeWithCustom(&rd->nodeId.nodeId,
                                      client->config.customDataTypes) != NULL;
        const UA_Boolean collectUnknown =
            (!isKnown && rd->nodeId.nodeId.namespaceIndex != 0);

        /* Unknown entries are collected exactly once in the tree. */
        if(collectUnknown && ZIP_FIND(NodeIdTree, tree, &rd->nodeId.nodeId))
            continue;

        if(collectUnknown) {
            /* Create an entry for unknown types */
            NodeIdTreeEntry *entry = (NodeIdTreeEntry*)UA_malloc(sizeof(NodeIdTreeEntry));
            if(!entry) {
                res = UA_STATUSCODE_BADOUTOFMEMORY;
                break;
            }
            *entry = (NodeIdTreeEntry){{NULL, NULL}, rd->nodeId.nodeId, rd->displayName
                .text, UA_NODEID_NULL};

            ZIP_INSERT(NodeIdTree, tree, entry);
            (*treeSize)++;
        }

        /* Recurse */
        res = browseDataTypesRecursive(client, tree, treeSize, rd->nodeId.nodeId);

        if(collectUnknown) {
            /* Don't double-free moved values for unknown entries */
            UA_NodeId_init(&rd->nodeId.nodeId);
            UA_String_init(&rd->displayName.text);
        }
    }

    UA_BrowseResult_clear(&br);
    return res;
}

/* Map a namespace-0 builtin hint NodeId to the BuiltInType id used in
 * SimpleTypeDescription (typeKind + 1). Returns true only for simple
 * builtins that can be represented as UA_DATATYPEKIND_* <= DIAGNOSTICINFO. */
static UA_Boolean
simpleBuiltInTypeFromHint(const UA_NodeId *builtinHint,
                          const UA_DataTypeArray *customTypes,
                          UA_Byte *builtInType) {
    if(builtinHint->namespaceIndex != 0 ||
       builtinHint->identifierType != UA_NODEIDTYPE_NUMERIC)
        return false;

    const UA_DataType *baseType =
        UA_findDataTypeWithCustom(builtinHint, customTypes);
    if(!baseType)
        return false;
    if(baseType->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return false;

    *builtInType = (UA_Byte)(baseType->typeKind + 1);
    return true;
}

/* Entry status for importer lifecycle */
typedef enum EntryStatus {
    ENTRY_PENDING = 0,
    ENTRY_UNUSABLE = 1,
    ENTRY_RETRYABLE = 2,
    ENTRY_PROCESSED = 3
} EntryStatus;

static UA_StatusCode
getRemoteDataTypes(UA_Client *client, UA_ReadRequest *req,
                   UA_NodeId *builtinHints,
                   UA_DataTypeArray **outCustomTypes) {
    UA_ReadResponse rr = UA_Client_Service_read(client, *req);
    UA_StatusCode res = rr.responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD && rr.resultsSize != req->nodesToReadSize) {
        res = UA_STATUSCODE_BADINTERNALERROR;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_clear(&rr);
        return res;
    }

    /* Allocate the arrays */
    size_t typesSize = req->nodesToReadSize / 2;
    UA_DataTypeArray *dta = (UA_DataTypeArray *)UA_calloc(1, sizeof(UA_DataTypeArray));
    if(!dta) {
        UA_ReadResponse_clear(&rr);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    dta->cleanup = true;
    dta->types = (UA_DataType *)UA_calloc(typesSize, sizeof(UA_DataType));
    if(!dta->types) {
        UA_ReadResponse_clear(&rr);
        UA_cleanupDataTypeWithCustom(dta);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Track entries' importer lifecycle state. Initialize to PENDING. */
    EntryStatus *entryStatus = (EntryStatus *)UA_calloc(typesSize, sizeof(EntryStatus));
    if(!entryStatus) {
        UA_ReadResponse_clear(&rr);
        UA_cleanupDataTypeWithCustom(dta);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(entryStatus, 0, typesSize * sizeof(EntryStatus));

    size_t unresolved = 0;
    for(size_t i = 0; i < typesSize; i++) {
        UA_DataValue *typeDef = &rr.results[i];
        UA_DataValue *typeName = &rr.results[i + typesSize];
        UA_NodeId *builtinHint = &builtinHints[i];
        /* A bad DataTypeDefinition status cannot become importable in later passes. */
        if(typeDef->hasStatus && typeDef->status != UA_STATUSCODE_GOOD) {
            if(typeDef->status == UA_STATUSCODE_BADATTRIBUTEIDINVALID) {
                /* Fallback for simple subtypes (e.g. String subtypes):
                 * build a SimpleTypeDescription from the builtin hint. */
                UA_Byte simpleBuiltInType = 0;
                if(simpleBuiltInTypeFromHint(builtinHint, client->config.customDataTypes,
                                             &simpleBuiltInType) &&
                   typeName->hasValue &&
                   UA_Variant_hasScalarType(&typeName->value,
                                            &UA_TYPES[UA_TYPES_QUALIFIEDNAME])) {

                    UA_QualifiedName *name = (UA_QualifiedName *)typeName->value.data;

                    UA_SimpleTypeDescription simpleDescr;
                    UA_SimpleTypeDescription_init(&simpleDescr);
                    simpleDescr.dataTypeId = req->nodesToRead[i].nodeId;
                    simpleDescr.name = *name;
                    simpleDescr.baseDataType = *builtinHint;
                    simpleDescr.builtInType = simpleBuiltInType;

                    UA_ExtensionObject eo;
                    UA_ExtensionObject_setValue(
                        &eo, &simpleDescr, &UA_TYPES[UA_TYPES_SIMPLETYPEDESCRIPTION]);

                    UA_DataTypeArray lookupTypes = *dta;
                    lookupTypes.next = client->config.customDataTypes;

                    UA_StatusCode simpleRes = UA_DataType_fromDescription(
                        &dta->types[dta->typesSize], &eo, &lookupTypes);
                    if(simpleRes == UA_STATUSCODE_GOOD) {
                        UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                                    "Added DataType %Q (%N) to the DataTypeArray", *name,
                                    req->nodesToRead[i].nodeId);
                        dta->typesSize++;
                        entryStatus[i] = ENTRY_PROCESSED;
                        continue;
                    }

                    UA_LOG_ERROR(
                        client->config.logging, UA_LOGCATEGORY_CLIENT,
                        "Could not add simple DataType %Q (%N) with status code %s",
                        *name, req->nodesToRead[i].nodeId, UA_StatusCode_name(simpleRes));
                }
                UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                             "Skipping DataType %N: DataTypeDefinition status is %s "
                             "(builtin hint: %N)",
                             req->nodesToRead[i].nodeId,
                             UA_StatusCode_name(typeDef->status), *builtinHint);
            } else {
                UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                             "Skipping DataType %N: DataTypeDefinition status is %s",
                             req->nodesToRead[i].nodeId,
                             UA_StatusCode_name(typeDef->status));
            }
            entryStatus[i] = ENTRY_UNUSABLE;
            continue;
        }

        /* Only structure and enum definitions are handled by this importer path. */
        if(!typeDef->hasValue ||
           (!UA_Variant_hasScalarType(&typeDef->value,
                                      &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]) &&
            !UA_Variant_hasScalarType(&typeDef->value,
                                      &UA_TYPES[UA_TYPES_ENUMDEFINITION]))) {
            UA_LOG_ERROR(
                client->config.logging, UA_LOGCATEGORY_CLIENT,
                "Skipping DataType %N: unsupported or missing DataTypeDefinition value",
                req->nodesToRead[i].nodeId);
            entryStatus[i] = ENTRY_UNUSABLE;
            continue;
        }

        /* A bad BrowseName status means we cannot construct a valid type descriptor. */
        if(typeName->hasStatus && typeName->status != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Skipping DataType %N: BrowseName status is %s",
                         req->nodesToRead[i].nodeId,
                         UA_StatusCode_name(typeName->status));
            entryStatus[i] = ENTRY_UNUSABLE;
            continue;
        }

        /* A missing or invalid BrowseName prevents identifying this datatype entry. */
        if(!typeName->hasValue ||
           !UA_Variant_hasScalarType(&typeName->value,
                                     &UA_TYPES[UA_TYPES_QUALIFIEDNAME])) {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Skipping DataType %N: missing or invalid BrowseName value",
                         req->nodesToRead[i].nodeId);
            entryStatus[i] = ENTRY_UNUSABLE;
            continue;
        }

        unresolved++;
    }

    while(unresolved > 0) {
        size_t addedThisPass = 0;

        for(size_t i = 0; i < typesSize; i++) {
            if(entryStatus[i] != ENTRY_PENDING)
                continue;

            UA_DataValue *typeDef = &rr.results[i];
            UA_DataValue *typeName = &rr.results[i + typesSize];
            UA_QualifiedName *name = (UA_QualifiedName *)typeName->value.data;
            UA_NodeId *builtinHint = &builtinHints[i];

            UA_ExtensionObject eo;
            UA_StructureDescription structDescr;
            UA_EnumDescription enumDescr;
            if(UA_Variant_hasScalarType(&typeDef->value,
                                        &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION])) {
                UA_StructureDefinition *sd =
                    (UA_StructureDefinition *)typeDef->value.data;
                structDescr.structureDefinition = *sd;
                structDescr.dataTypeId = req->nodesToRead[i].nodeId;
                structDescr.name = *name;
                UA_ExtensionObject_setValue(&eo, &structDescr,
                                            &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
            } else {
                UA_EnumDefinition *ed = (UA_EnumDefinition *)typeDef->value.data;
                enumDescr.dataTypeId = req->nodesToRead[i].nodeId;
                enumDescr.name = *name;
                enumDescr.enumDefinition = *ed;
                /* Default: regular Int32 enumeration */
                enumDescr.builtInType = UA_DATATYPEKIND_INT32 + 1;
                /* If the builtin hint points to a numeric OptionSet base type,
                 * override builtInType so fromEnumDescription uses the right storage.
                 * OPC UA BuiltInType IDs: Byte=3, UInt16=5, UInt32=7, UInt64=9 */
                if(builtinHints && !UA_NodeId_isNull(builtinHint) &&
                   builtinHint->namespaceIndex == 0 &&
                   builtinHint->identifierType == UA_NODEIDTYPE_NUMERIC) {
                    UA_UInt32 id = builtinHint->identifier.numeric;
                    if(id == UA_NS0ID_BYTE || id == UA_NS0ID_UINT16 ||
                       id == UA_NS0ID_UINT32 || id == UA_NS0ID_UINT64)
                        enumDescr.builtInType = (UA_Byte)id;
                }
                UA_ExtensionObject_setValue(&eo, &enumDescr,
                                            &UA_TYPES[UA_TYPES_ENUMDESCRIPTION]);
            }

            UA_DataTypeArray lookupTypes = *dta;
            lookupTypes.next = client->config.customDataTypes;

            res = UA_DataType_fromDescription(&dta->types[dta->typesSize], &eo,
                                              &lookupTypes);
            if(res == UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                            "Added DataType %Q (%N) to the DataTypeArray", *name,
                            req->nodesToRead[i].nodeId);
                dta->typesSize++;
                entryStatus[i] = ENTRY_PROCESSED;
                unresolved--;
                addedThisPass++;
                continue;
            }

            if(res == UA_STATUSCODE_BADNOTFOUND)
                continue;

            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Could not add the DataType %Q (%N) with status code %s", *name,
                         req->nodesToRead[i].nodeId, UA_StatusCode_name(res));
            entryStatus[i] = ENTRY_UNUSABLE;
            unresolved--;
        }

        if(addedThisPass == 0)
            break;
    }

    for(size_t i = 0; i < typesSize; i++) {
        if(entryStatus[i] != ENTRY_PENDING)
            continue;

        UA_DataValue *typeName = &rr.results[i + typesSize];
        UA_QualifiedName *name = (UA_QualifiedName *)typeName->value.data;
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Could not add the DataType %Q (%N) with status code %s", *name,
                     req->nodesToRead[i].nodeId,
                     UA_StatusCode_name(UA_STATUSCODE_BADNOTFOUND));
    }

    UA_free(entryStatus);

    *outCustomTypes = dta;

    UA_ReadResponse_clear(&rr);
    UA_ReadRequest_clear(req);
    if(dta->typesSize == 0) {
        UA_free(dta->types);
        dta->types = NULL;
    }
    return UA_STATUSCODE_GOOD;
}

typedef struct {
    UA_ReadValueId **rvi;
    UA_NodeId *hints;
    size_t idx;
} SetNodesCtx;

static void *
setNodesToRead(void *context, NodeIdTreeEntry *elm) {
    SetNodesCtx *ctx = (SetNodesCtx*)context;
    /* deep-copy NodeId so elm can be freed later */
    UA_NodeId_init(&(*ctx->rvi)->nodeId);
    (void)UA_NodeId_copy(&elm->nodeId, &(*ctx->rvi)->nodeId);
    (*ctx->rvi)->attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    /* copy builtin hint into parallel array only if present */
    if(!UA_NodeId_isNull(&elm->builtinHint))
        (void)UA_NodeId_copy(&elm->builtinHint, &ctx->hints[ctx->idx]);
    ctx->idx++;
    (*ctx->rvi)++;
    return NULL;
}

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_getRemoteDataTypes(UA_Client *client,
                             size_t dataTypesNodesSize,
                             const UA_NodeId *dataTypesNodes,
                             UA_DataTypeArray **customTypes) {
    if(!client || !customTypes)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_NodeId *builtinHints = NULL;
    UA_ReadValueId *nodesToRead = NULL;
    NodeIdTree tree;
    ZIP_INIT(&tree);
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);

    /* req.nodesToRead has twice the normal length. Because we read the
     * DataTypeDescription and the DisplayName in the same request. */

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(dataTypesNodesSize == 0) {
        /* Browse to find unknown datatypes in the information model */

        /* Get all unknown DataType NodeIds */
        size_t treeSize = 0;
        /* Discover the complete custom DataType subtree from BaseDataType. We need
         * the full subtree to be able to get...
         * - Enumerations
         * - Structures
         * - OptionSets (Unsigned integer subtypes)
         * - Subtypes of built-in types (e.g. String subtypes)
         */
        res = browseDataTypesRecursive(client, &tree, &treeSize, UA_NS0ID(BASEDATATYPE));
        if(res != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        if(treeSize == 0) {
            res = UA_STATUSCODE_GOOD;
            goto cleanup;
        }

        /* Try to resolve builtin supertypes (hints) for discovered datatypes. */
        ZIP_ITER(NodeIdTree, &tree, populateBuiltinHint, client);

        /* Read the descriptions of the unknown types */
        nodesToRead = (UA_ReadValueId*)UA_calloc(treeSize * 2, sizeof(UA_ReadValueId));
        if(!nodesToRead) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        UA_ReadValueId *tmp = nodesToRead;
        /* allocate parallel array for builtin hints */
        builtinHints = (UA_NodeId*)UA_calloc(treeSize, sizeof(UA_NodeId));
        if(!builtinHints) {
            UA_free(nodesToRead);
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        for(size_t _i = 0; _i < treeSize; _i++)
            UA_NodeId_init(&builtinHints[_i]);

        SetNodesCtx ctx = { &tmp, builtinHints, 0 };
        ZIP_ITER(NodeIdTree, &tree, setNodesToRead, &ctx);
        req.nodesToRead = nodesToRead;
        req.nodesToReadSize = treeSize;

        /* entries not needed anymore; actual deletion moved to cleanup */
    } else {
        /* Prepare the ReadRequest for the provided NodeIds */
        req.nodesToRead = (UA_ReadValueId*)
            UA_Array_new(dataTypesNodesSize * 2, &UA_TYPES[UA_TYPES_READVALUEID]);
        if(!req.nodesToRead) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        req.nodesToReadSize = dataTypesNodesSize;

        /* allocate parallel array for builtin hints for provided NodeIds */
        builtinHints = (UA_NodeId*)UA_calloc(dataTypesNodesSize, sizeof(UA_NodeId));
        if(!builtinHints) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        for(size_t _i = 0; _i < dataTypesNodesSize; _i++)
            UA_NodeId_init(&builtinHints[_i]);

        for(size_t i = 0; i < dataTypesNodesSize; i++) {
            /* Only numeric NodeIds are supported for DataType NodeIds in this path. */
            if(dataTypesNodes[i].identifierType != UA_NODEIDTYPE_NUMERIC) {
                UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                             "Unsupported NodeId identifier type for DataType NodeId: %d",
                             (int)dataTypesNodes[i].identifierType);
                res = UA_STATUSCODE_BADINVALIDARGUMENT;
                goto cleanup;
            }

            req.nodesToRead[i].attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
            res |= UA_NodeId_copy(&dataTypesNodes[i], &req.nodesToRead[i].nodeId);

            /* Try to resolve a builtin supertype hint for the provided NodeId. */
            const UA_StatusCode hintRes = resolveBuiltinHint(client, &dataTypesNodes[i],
                &builtinHints[i]);
            if(hintRes != UA_STATUSCODE_GOOD) {
                UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                             "Could not resolve builtin hint for provided DataType %N: %s",
                             dataTypesNodes[i], UA_StatusCode_name(hintRes));
            }
        }

        if(res != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }
    }

    /* Prepare reading the BrowseName attributes additionally */
    for(size_t i = 0; i < req.nodesToReadSize; i++) {
        res |= UA_NodeId_copy(&req.nodesToRead[i].nodeId,
                              &req.nodesToRead[i + req.nodesToReadSize].nodeId);
        req.nodesToRead[i + req.nodesToReadSize].attributeId = UA_ATTRIBUTEID_BROWSENAME;
    }
    req.nodesToReadSize *= 2;
    if(res != UA_STATUSCODE_GOOD) {
        goto cleanup;
    }

    /* Read and convert the DataTypeDefinitions */
    res = getRemoteDataTypes(client, &req, builtinHints, customTypes);

cleanup:
    /* free any builtinHints we allocated (array length = nodesToReadSize/2) */
    if(builtinHints) {
        const size_t hintCount = req.nodesToReadSize / 2;
        for(size_t _i = 0; _i < hintCount; _i++)
            UA_NodeId_clear(&builtinHints[_i]);
        UA_free(builtinHints);
        builtinHints = NULL;
    }
    ZIP_ITER(NodeIdTree, &tree, deleteNodeIdEntry, NULL);
    UA_ReadRequest_clear(&req);
    return res;
}
