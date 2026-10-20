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

/* Forget the Methods that were deleted together with their LogObject */
static void
pruneBoundMethods(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    size_t i = 0;
    while(i < lod->boundMethodsSize) {
        UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
        if(UA_Server_readNodeClass(server, lod->boundMethods[i].methodId, &nc) !=
           UA_STATUSCODE_BADNODEIDUNKNOWN) {
            i++;
            continue;
        }
        UA_NodeId_clear(&lod->boundMethods[i].methodId);
        lod->boundMethods[i] = lod->boundMethods[lod->boundMethodsSize - 1];
        lod->boundMethodsSize--;
    }
}

static UA_StatusCode
bindMethod(LogObjectDriver *lod, const UA_NodeId *methodId, UA_MethodCallback callback) {
    UA_Server *server = lod->driver.server;
    UA_MethodCallback previous = NULL;
    UA_StatusCode res = UA_Server_getMethodNodeCallback(server, *methodId, &previous);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* A Method that was deleted and created again with the same NodeId is
     * bound anew */
    LogObjectBoundMethod *bm = NULL;
    for(size_t i = 0; i < lod->boundMethodsSize; i++) {
        if(UA_NodeId_equal(&lod->boundMethods[i].methodId, methodId)) {
            bm = &lod->boundMethods[i];
            break;
        }
    }
    if(bm && previous == bm->callback)
        return UA_STATUSCODE_GOOD;

    /* A copy of the Method of the type (copyMethodsOnInstances) inherits the
     * callback of the driver. That is not restored when the driver stops. */
    if(previous == logObjectGetRecordsMethod ||
       previous == logObjectReleaseContinuationPointMethod)
        previous = NULL;

    if(!bm) {
        pruneBoundMethods(lod);
        bm = (LogObjectBoundMethod*)
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
        lod->boundMethodsSize++;
    } else {
        res = UA_Server_setMethodNodeCallback(server, *methodId, callback);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    bm->callback = callback;
    bm->previous = previous;
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

/*************/
/* Instances */
/*************/

/* Every instance of the LogObjectType and its subtypes is a LogObject: also
 * the instances of companion specifications loaded from a nodeset. The driver
 * installs the lifecycle of the types to register new instances and scans for
 * the existing ones when it starts. */

/* An InstanceDeclaration below an ObjectType or VariableType describes the
 * instances of that type and is not a LogObject itself */
static UA_Boolean
isInstanceDeclaration(UA_Server *server, const UA_NodeId *nodeId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *nodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.referenceTypeId = UA_NS0ID(HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE;
    size_t resultsSize = 0;
    UA_ExpandedNodeId *results = NULL;
    UA_StatusCode res = UA_Server_browseRecursive(server, &bd, &resultsSize, &results);
    UA_Array_delete(results, resultsSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return res == UA_STATUSCODE_GOOD && resultsSize > 0;
}

/* The settings of an instance come from its Properties if they are present
 * and valid. Otherwise the defaults of the driver apply. */
static UA_LogObjectSettings
readInstanceSettings(LogObjectDriver *lod, const UA_NodeId *nodeId) {
    UA_Server *server = lod->driver.server;
    UA_LogObjectSettings s = lod->defaultSettings;
    UA_Variant v;
    UA_Variant_init(&v);
    if(UA_Server_readObjectProperty(server, *nodeId, UA_QUALIFIEDNAME(0, "MaxRecords"),
                                    &v) == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT32]) &&
       *(UA_UInt32*)v.data > 0)
        s.maxRecords = *(UA_UInt32*)v.data;
    UA_Variant_clear(&v);
    if(UA_Server_readObjectProperty(server, *nodeId,
                                    UA_QUALIFIEDNAME(0, "MaxStorageDuration"),
                                    &v) == UA_STATUSCODE_GOOD &&
       v.data && UA_Variant_isScalar(&v) && v.type->typeKind == UA_DATATYPEKIND_DOUBLE) {
        /* Zero is invalid (Part 26, 5.2) */
        UA_Double d = *(UA_Double*)v.data;
        if(logObjectValidDuration(d))
            s.maxStorageDuration = d;
    }
    UA_Variant_clear(&v);
    if(UA_Server_readObjectProperty(server, *nodeId,
                                    UA_QUALIFIEDNAME(0, "MinimumSeverity"),
                                    &v) == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT16])) {
        UA_UInt16 severity = *(UA_UInt16*)v.data;
        if(severity >= 1 && severity <= 1000)
            s.minimumSeverity = severity;
    }
    UA_Variant_clear(&v);
    return s;
}

static UA_StatusCode
registerInstance(LogObjectDriver *lod, const UA_NodeId *nodeId) {
    UA_Server *server = lod->driver.server;
    if(findLogObjectEntry(lod, nodeId) || isInstanceDeclaration(server, nodeId))
        return UA_STATUSCODE_GOOD;
    LogObjectEntry *entry = (LogObjectEntry*)UA_calloc(1, sizeof(LogObjectEntry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode res = UA_NodeId_copy(nodeId, &entry->nodeId);
    if(res == UA_STATUSCODE_GOOD) {
        entry->settings = readInstanceSettings(lod, nodeId);
        res = registerLogObjectNode(lod, entry);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&entry->nodeId);
        UA_free(entry);
    }
    return res;
}

static LogObjectTypeLifecycle *
findTypeLifecycle(LogObjectDriver *lod, const UA_NodeId *typeId) {
    for(size_t i = 0; i < lod->typesSize; i++) {
        if(UA_NodeId_equal(&lod->types[i].typeId, typeId))
            return &lod->types[i];
    }
    return NULL;
}

/* The lifecycle of the application chains back to the driver if it was set
 * with the driver's lifecycle (UA_Server_getNodeTypeLifecycle) as the next
 * one. The driver does not call it again for the same node. */
static UA_Boolean
isSameNode(const UA_NodeId *current, const UA_NodeId *nodeId) {
    return current && UA_NodeId_equal(current, nodeId);
}

/* The constructor of the application runs first, it may replace the context.
 * The stopped driver only forwards to it. */
static UA_StatusCode
logObjectConstructor(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *typeNodeId,
                     void *typeNodeContext, const UA_NodeId *nodeId,
                     void **nodeContext) {
    LogObjectDriver *lod = findLogObjectDriver(server);
    if(!lod)
        return UA_STATUSCODE_GOOD;
    LogObjectTypeLifecycle *tl = findTypeLifecycle(lod, typeNodeId);
    if(tl && tl->application.constructor && !isSameNode(lod->constructing, nodeId)) {
        const UA_NodeId *outer = lod->constructing;
        lod->constructing = nodeId;
        UA_StatusCode res =
            tl->application.constructor(server, sessionId, sessionContext, typeNodeId,
                                        typeNodeContext, nodeId, nodeContext);
        lod->constructing = outer;
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    if(lod->driver.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_GOOD;
    /* A failed registration leaves the node in place, unserved. Only running
     * out of memory fails the creation of the node. */
    UA_StatusCode res = registerInstance(lod, nodeId);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "LogObject: Could not serve the LogObject %N (%s)",
                       *nodeId, UA_StatusCode_name(res));
    return (res == UA_STATUSCODE_BADOUTOFMEMORY) ? res : UA_STATUSCODE_GOOD;
}

/* A deleted LogObject loses its records. The destructor of the application
 * runs last. */
static void
logObjectDestructor(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *typeNodeId,
                    void *typeNodeContext, const UA_NodeId *nodeId,
                    void **nodeContext) {
    LogObjectDriver *lod = findLogObjectDriver(server);
    if(!lod)
        return;
    LogObjectEntry *entry = findLogObjectEntry(lod, nodeId);
    if(entry) {
        if(entry->isServerLog) {
            removeLogCapture(lod);
            lod->serverLogRegistered = false;
            lod->createdServerLog = false;
        }
        unregisterLogObjectNode(lod, entry, true);
        if(!entry->isServerLog) {
            UA_NodeId_clear(&entry->nodeId);
            UA_free(entry);
        }
    }
    LogObjectTypeLifecycle *tl = findTypeLifecycle(lod, typeNodeId);
    if(tl && tl->application.destructor && !isSameNode(lod->destructing, nodeId)) {
        const UA_NodeId *outer = lod->destructing;
        lod->destructing = nodeId;
        tl->application.destructor(server, sessionId, sessionContext, typeNodeId,
                                   typeNodeContext, nodeId, nodeContext);
        lod->destructing = outer;
    }
}

/* Install the lifecycle on a type that is not served yet */
static UA_StatusCode
installTypeLifecycle(LogObjectDriver *lod, const UA_NodeId *typeId) {
    if(findTypeLifecycle(lod, typeId))
        return UA_STATUSCODE_GOOD;
    UA_Server *server = lod->driver.server;
    UA_NodeTypeLifecycle application;
    UA_StatusCode res = UA_Server_getNodeTypeLifecycle(server, *typeId, &application);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LogObjectTypeLifecycle *types = (LogObjectTypeLifecycle*)
        UA_realloc(lod->types, (lod->typesSize + 1) * sizeof(LogObjectTypeLifecycle));
    if(!types)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    lod->types = types;
    LogObjectTypeLifecycle *tl = &lod->types[lod->typesSize];
    res = UA_NodeId_copy(typeId, &tl->typeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    tl->application = application;
    UA_NodeTypeLifecycle lifecycle = application;
    lifecycle.constructor = logObjectConstructor;
    lifecycle.destructor = logObjectDestructor;
    res = UA_Server_setNodeTypeLifecycle(server, *typeId, lifecycle);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&tl->typeId);
        return res;
    }
    lod->typesSize++;
    return UA_STATUSCODE_GOOD;
}

/* A lifecycle that the application set after the driver started may chain to
 * the driver's. It stays in place and the stopped driver forwards to the
 * lifecycle the application had set before. A restart does not install the
 * driver's lifecycle on that type again. */
void
restoreLogObjectTypeLifecycles(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;
    size_t kept = 0;
    for(size_t i = 0; i < lod->typesSize; i++) {
        LogObjectTypeLifecycle *tl = &lod->types[i];
        UA_NodeTypeLifecycle current;
        UA_StatusCode res = UA_Server_getNodeTypeLifecycle(server, tl->typeId, &current);
        if(res == UA_STATUSCODE_GOOD && current.constructor != logObjectConstructor) {
            lod->types[kept++] = *tl;
            continue;
        }
        if(res == UA_STATUSCODE_GOOD)
            UA_Server_setNodeTypeLifecycle(server, tl->typeId, tl->application);
        UA_NodeId_clear(&tl->typeId);
    }
    lod->typesSize = kept;
    if(kept == 0) {
        UA_free(lod->types);
        lod->types = NULL;
    }
}

void
clearLogObjectTypeLifecycles(LogObjectDriver *lod) {
    for(size_t i = 0; i < lod->typesSize; i++)
        UA_NodeId_clear(&lod->types[i].typeId);
    UA_free(lod->types);
    lod->types = NULL;
    lod->typesSize = 0;
}

static UA_StatusCode
browseAll(UA_Server *server, const UA_NodeId *start, UA_BrowseDirection direction,
          const UA_NodeId referenceType, UA_UInt32 nodeClassMask,
          size_t *resultsSize, UA_ExpandedNodeId **results) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *start;
    bd.browseDirection = direction;
    bd.referenceTypeId = referenceType;
    bd.includeSubtypes = false;
    bd.nodeClassMask = nodeClassMask;
    return UA_Server_browseRecursive(server, &bd, resultsSize, results);
}

UA_StatusCode
serveLogObjectInstances(LogObjectDriver *lod) {
    UA_Server *server = lod->driver.server;

    /* The LogObjectType and its subtypes. Lifecycles are not inherited, every
     * type gets its own. */
    size_t subtypesSize = 0;
    UA_ExpandedNodeId *subtypes = NULL;
    UA_NodeId logObjectType = UA_NS0ID(LOGOBJECTTYPE);
    UA_StatusCode res = browseAll(server, &logObjectType, UA_BROWSEDIRECTION_FORWARD,
                                  UA_NS0ID(HASSUBTYPE), UA_NODECLASS_OBJECTTYPE,
                                  &subtypesSize, &subtypes);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = installTypeLifecycle(lod, &logObjectType);
    for(size_t i = 0; res == UA_STATUSCODE_GOOD && i < subtypesSize; i++) {
        if(UA_ExpandedNodeId_isLocal(&subtypes[i]))
            res = installTypeLifecycle(lod, &subtypes[i].nodeId);
    }
    UA_Array_delete(subtypes, subtypesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

    /* The Methods of the type are referenced from the instances that do not
     * have own Methods */
    UA_NodeId getRecords = UA_NS0ID(LOGOBJECTTYPE_GETRECORDS);
    UA_NodeId release = UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT);
    if(res == UA_STATUSCODE_GOOD)
        res = bindMethod(lod, &getRecords, logObjectGetRecordsMethod);
    if(res == UA_STATUSCODE_GOOD)
        res = bindMethod(lod, &release, logObjectReleaseContinuationPointMethod);

    /* Register the existing instances of every type */
    for(size_t i = 0; res == UA_STATUSCODE_GOOD && i < lod->typesSize; i++) {
        size_t instancesSize = 0;
        UA_ExpandedNodeId *instances = NULL;
        res = browseAll(server, &lod->types[i].typeId, UA_BROWSEDIRECTION_INVERSE,
                        UA_NS0ID(HASTYPEDEFINITION), UA_NODECLASS_OBJECT,
                        &instancesSize, &instances);
        for(size_t j = 0; res == UA_STATUSCODE_GOOD && j < instancesSize; j++) {
            if(UA_ExpandedNodeId_isLocal(&instances[j]))
                res = registerInstance(lod, &instances[j].nodeId);
        }
        UA_Array_delete(instances, instancesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    }
    return res;
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
