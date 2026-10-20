/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <string.h>

/* The nodes of OPC UA Part 26. The driver creates the well-known ServerLog
 * (7.2) and serves the LogObjects in the information model. Everything it
 * changes is undone when it stops. */

/*************/
/* ServerLog */
/*************/

static UA_StatusCode
addProperty(UA_Server *server, const UA_NodeId propertyId, const UA_NodeId parentId,
            char *name, void *value, const UA_DataType *type) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.dataType = type->typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalar(&attr.value, value, type);
    return UA_Server_addVariableNode(server, propertyId, parentId,
                                     UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, name),
                                     UA_NS0ID(PROPERTYTYPE), attr, NULL, NULL);
}

/* The Server instance of the MaxLogObjectContinuationPoints Property (Part 5,
 * 6.3.2) is not part of the standard nodeset */
static UA_StatusCode
createMaxContinuationPoints(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    UA_UInt16 max = lod->maxContinuationPoints;
    UA_NodeId propertyId = UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS);
    UA_StatusCode res =
        addProperty(server, propertyId, UA_NS0ID(SERVER_SERVERCAPABILITIES),
                    "MaxLogObjectContinuationPoints", &max, &UA_TYPES[UA_TYPES_UINT16]);
    if(res == UA_STATUSCODE_GOOD) {
        lod->createdMaxContinuationPoints = true;
        return UA_STATUSCODE_GOOD;
    }
    if(res != UA_STATUSCODE_BADNODEIDEXISTS)
        return res;
    UA_Variant v;
    UA_Variant_setScalar(&v, &max, &UA_TYPES[UA_TYPES_UINT16]);
    return UA_Server_writeValue(server, propertyId, v);
}

/* Create the ServerLog with the NodeIds of the standard nodeset. Its GetRecords
 * Method and the Properties are added before the Object is finished, so the
 * instantiation of the LogObjectType does not create them again. */
static UA_StatusCode
createServerLog(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    UA_LogObjectSettings *s = &lod->serverLogSettings;
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    oa.displayName = UA_LOCALIZEDTEXT("", "ServerLog");
    UA_StatusCode res =
        UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NS0ID(SERVERLOG),
                                UA_NS0ID(SERVER), UA_NS0ID(HASCOMPONENT),
                                UA_QUALIFIEDNAME(0, "ServerLog"), UA_NS0ID(LOGOBJECTTYPE),
                                &oa, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    lod->createdServerLog = true;

    /* The GetRecords Method with the Arguments of the type */
    UA_Variant in, out;
    UA_Variant_init(&in);
    UA_Variant_init(&out);
    res = UA_Server_readValue(server, UA_NS0ID(LOGOBJECTTYPE_GETRECORDS_INPUTARGUMENTS), &in);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_readValue(server, UA_NS0ID(LOGOBJECTTYPE_GETRECORDS_OUTPUTARGUMENTS),
                                  &out);
    if(res == UA_STATUSCODE_GOOD &&
       (in.type != &UA_TYPES[UA_TYPES_ARGUMENT] || out.type != &UA_TYPES[UA_TYPES_ARGUMENT]))
        res = UA_STATUSCODE_BADINTERNALERROR;
    if(res == UA_STATUSCODE_GOOD) {
        UA_MethodAttributes ma = UA_MethodAttributes_default;
        ma.displayName = UA_LOCALIZEDTEXT("", "GetRecords");
        ma.executable = true;
        ma.userExecutable = true;
        res = UA_Server_addMethodNodeEx(server, UA_NS0ID(SERVERLOG_GETRECORDS),
                                        UA_NS0ID(SERVERLOG), UA_NS0ID(HASCOMPONENT),
                                        UA_QUALIFIEDNAME(0, "GetRecords"), ma, NULL,
                                        in.arrayLength, (const UA_Argument*)in.data,
                                        UA_NS0ID(SERVERLOG_GETRECORDS_INPUTARGUMENTS), NULL,
                                        out.arrayLength, (const UA_Argument*)out.data,
                                        UA_NS0ID(SERVERLOG_GETRECORDS_OUTPUTARGUMENTS), NULL,
                                        NULL, NULL);
    }
    UA_Variant_clear(&in);
    UA_Variant_clear(&out);

    /* The Properties. Zero is not a valid MaxStorageDuration (Part 26, 5.2).
     * Without a time-based limit the optional Property is not exposed. */
    if(res == UA_STATUSCODE_GOOD)
        res = addProperty(server, UA_NS0ID(SERVERLOG_MAXRECORDS), UA_NS0ID(SERVERLOG),
                          "MaxRecords", &s->maxRecords, &UA_TYPES[UA_TYPES_UINT32]);
    if(res == UA_STATUSCODE_GOOD && s->maxStorageDuration > 0.0)
        res = addProperty(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), UA_NS0ID(SERVERLOG),
                          "MaxStorageDuration", &s->maxStorageDuration,
                          &UA_TYPES[UA_TYPES_DURATION]);
    if(res == UA_STATUSCODE_GOOD)
        res = addProperty(server, UA_NS0ID(SERVERLOG_MINIMUMSEVERITY), UA_NS0ID(SERVERLOG),
                          "MinimumSeverity", &s->minimumSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_addNode_finish(server, UA_NS0ID(SERVERLOG));
    return res;
}

UA_StatusCode
createServerLogNodes(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode res = UA_Server_readNodeClass(server, UA_NS0ID(LOGOBJECTTYPE), &nc);
    if(res != UA_STATUSCODE_GOOD || nc != UA_NODECLASS_OBJECTTYPE)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    res = createMaxContinuationPoints(lod);
    if(res != UA_STATUSCODE_GOOD || !lod->serverLogEnabled)
        return res;
    /* An existing ServerLog of the application is served as it is */
    if(UA_Server_readNodeClass(server, UA_NS0ID(SERVERLOG), &nc) == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOOD;
    return createServerLog(lod);
}

void
deleteServerLogNodes(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    if(lod->createdServerLog)
        UA_Server_deleteNode(server, UA_NS0ID(SERVERLOG), true);
    if(lod->createdMaxContinuationPoints)
        UA_Server_deleteNode(server,
            UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS), true);
    lod->createdServerLog = false;
    lod->createdMaxContinuationPoints = false;
}

/*****************/
/* Bound Methods */
/*****************/

static UA_StatusCode
bindMethod(LogObjectDriver *lod, const UA_NodeId *methodId, UA_MethodCallback callback) {
    for(size_t i = 0; i < lod->boundMethodsSize; i++) {
        if(UA_NodeId_equal(&lod->boundMethods[i].methodId, methodId))
            return UA_STATUSCODE_GOOD;
    }
    UA_Server *server = lod->driver.server;
    UA_MethodCallback previous = NULL;
    UA_StatusCode res = UA_Server_getMethodNodeCallback(server, *methodId, &previous);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LogObjectBoundMethod *bm = (LogObjectBoundMethod*)
        UA_realloc(lod->boundMethods,
                   (lod->boundMethodsSize + 1) * sizeof(LogObjectBoundMethod));
    if(!bm)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    lod->boundMethods = bm;
    bm = &lod->boundMethods[lod->boundMethodsSize];
    res = UA_NodeId_copy(methodId, &bm->methodId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_Server_setMethodNodeCallback(server, *methodId, callback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&bm->methodId);
        return res;
    }
    bm->callback = callback;
    bm->previous = previous;
    lod->boundMethodsSize++;
    return UA_STATUSCODE_GOOD;
}

/* Restore the previous callbacks where the driver's callback is still set */
void
unbindLogObjectMethods(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    for(size_t i = 0; i < lod->boundMethodsSize; i++) {
        LogObjectBoundMethod *bm = &lod->boundMethods[i];
        UA_MethodCallback current = NULL;
        if(UA_Server_getMethodNodeCallback(server, bm->methodId, &current) ==
           UA_STATUSCODE_GOOD && current == bm->callback)
            UA_Server_setMethodNodeCallback(server, bm->methodId, bm->previous);
        UA_NodeId_clear(&bm->methodId);
    }
    UA_free(lod->boundMethods);
    lod->boundMethods = NULL;
    lod->boundMethodsSize = 0;
}

/* Serve the GetRecords and ReleaseContinuationPoint Methods of the LogObject.
 * The Methods are either own nodes of the instance or the nodes of the type
 * that are referenced from the instance. */
static UA_StatusCode
bindLogObjectMethods(LogObjectDriver *lod, const UA_NodeId *logObjectId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *logObjectId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_METHOD;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(lod->driver.server, 0, &bd);
    UA_StatusCode res = br.statusCode;
    UA_QualifiedName getRecords = UA_QUALIFIEDNAME(0, "GetRecords");
    UA_QualifiedName release = UA_QUALIFIEDNAME(0, "ReleaseContinuationPoint");
    for(size_t i = 0; res == UA_STATUSCODE_GOOD && i < br.referencesSize; i++) {
        const UA_ReferenceDescription *rd = &br.references[i];
        if(!UA_ExpandedNodeId_isLocal(&rd->nodeId))
            continue;
        if(UA_QualifiedName_equal(&rd->browseName, &getRecords))
            res = bindMethod(lod, &rd->nodeId.nodeId, logObjectGetRecordsMethod);
        else if(UA_QualifiedName_equal(&rd->browseName, &release))
            res = bindMethod(lod, &rd->nodeId.nodeId, logObjectReleaseContinuationPointMethod);
    }
    UA_BrowseResult_clear(&br);
    return res;
}

/****************/
/* Registration */
/****************/

static UA_Boolean
hasChild(UA_Server *server, const UA_NodeId *nodeId, char *name) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, name);
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *nodeId, 1, &qn);
    UA_Boolean found = (bpr.statusCode == UA_STATUSCODE_GOOD && bpr.targetsSize > 0);
    UA_BrowsePathResult_clear(&bpr);
    return found;
}

UA_StatusCode
registerLogObjectNode(LogObjectDriver *lod, LogObjectEntry *entry) {
    UA_Server *server = lod->driver.server;
    UA_StatusCode res =
        lod->backend.registerLogObject(server, lod->backend.context,
                                       &entry->nodeId, &entry->settings);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LIST_INSERT_HEAD(&lod->logObjects, entry, pointers);

    /* The Logs Folder resolves to all LogObjects of the Server (Part 26, 7.3) */
    UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
    if(UA_Server_readNodeClass(server, UA_NS0ID(LOGS), &nc) == UA_STATUSCODE_GOOD) {
        res = UA_Server_addReference(server, UA_NS0ID(LOGS), UA_NS0ID(ORGANIZES),
                                     UA_EXPANDEDNODEID_NODEID(entry->nodeId), true);
        if(res == UA_STATUSCODE_GOOD)
            entry->logsReferenceAdded = true;
        else if(res == UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            res = UA_STATUSCODE_GOOD;
    }

    /* The optional ReleaseContinuationPoint Method is not instantiated by
     * default. Reference the Method of the type, as done for the Methods of
     * instantiated Objects. */
    if(res == UA_STATUSCODE_GOOD && !hasChild(server, &entry->nodeId, "ReleaseContinuationPoint")) {
        res = UA_Server_addReference(server, entry->nodeId, UA_NS0ID(HASCOMPONENT),
            UA_EXPANDEDNODEID_NODEID(UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT)), true);
        if(res == UA_STATUSCODE_GOOD)
            entry->releaseReferenceAdded = true;
    }

    /* The Properties mirror the settings */
    if(res == UA_STATUSCODE_GOOD) {
        UA_Server_writeObjectProperty_scalar(server, entry->nodeId,
                                             UA_QUALIFIEDNAME(0, "MaxRecords"),
                                             &entry->settings.maxRecords,
                                             &UA_TYPES[UA_TYPES_UINT32]);
        UA_Server_writeObjectProperty_scalar(server, entry->nodeId,
                                             UA_QUALIFIEDNAME(0, "MinimumSeverity"),
                                             &entry->settings.minimumSeverity,
                                             &UA_TYPES[UA_TYPES_UINT16]);
        if(entry->settings.maxStorageDuration > 0.0)
            UA_Server_writeObjectProperty_scalar(server, entry->nodeId,
                                                 UA_QUALIFIEDNAME(0, "MaxStorageDuration"),
                                                 &entry->settings.maxStorageDuration,
                                                 &UA_TYPES[UA_TYPES_DURATION]);
        res = bindLogObjectMethods(lod, &entry->nodeId);
    }

    if(res != UA_STATUSCODE_GOOD)
        unregisterLogObjectNode(lod, entry, false);
    return res;
}

void
unregisterLogObjectNode(LogObjectDriver *lod, LogObjectEntry *entry,
                        UA_Boolean deleteRecords) {
    UA_Server *server = lod->driver.server;
    releaseContinuationPoints(lod, NULL, &entry->nodeId);
    if(entry->logsReferenceAdded)
        UA_Server_deleteReference(server, UA_NS0ID(LOGS), UA_NS0ID(ORGANIZES), true,
                                  UA_EXPANDEDNODEID_NODEID(entry->nodeId), true);
    if(entry->releaseReferenceAdded)
        UA_Server_deleteReference(server, entry->nodeId, UA_NS0ID(HASCOMPONENT), true,
            UA_EXPANDEDNODEID_NODEID(UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT)), true);
    entry->logsReferenceAdded = false;
    entry->releaseReferenceAdded = false;
    lod->backend.unregisterLogObject(server, lod->backend.context, &entry->nodeId,
                                     deleteRecords);
    LIST_REMOVE(entry, pointers);
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
