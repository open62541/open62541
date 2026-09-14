/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_LOGOBJECT

/* Wiring of the OPC UA Part 26 nodes in namespace zero */

static UA_StatusCode
writeLogObjectProperty(UA_Server *server, UA_UInt32 nodeId,
                       void *value, const UA_DataType *type) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setScalar(&v, value, type);
    return writeValueAttribute(server, UA_NODEID_NUMERIC(0, nodeId), &v);
}

/* The Server instance of the MaxLogObjectContinuationPoints Property
 * (i=19812) is not part of the standard nodeset. Create it below the
 * ServerCapabilities Object. */
static UA_StatusCode
addMaxContinuationPointsProperty(UA_Server *server) {
    UA_UInt16 max = server->config.maxLogObjectContinuationPoints;
    UA_NodeId propertyId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "MaxLogObjectContinuationPoints");
    attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalar(&attr.value, &max, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode res =
        addNode(server, UA_NODECLASS_VARIABLE, propertyId,
                UA_NS0ID(SERVER_SERVERCAPABILITIES), UA_NS0ID(HASPROPERTY),
                UA_QUALIFIEDNAME(0, "MaxLogObjectContinuationPoints"),
                UA_NS0ID(PROPERTYTYPE), &attr,
                &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], NULL, NULL);
    if(res == UA_STATUSCODE_BADNODEIDEXISTS)
        res = writeLogObjectProperty(server, propertyId.identifier.numeric,
                                     &max, &UA_TYPES[UA_TYPES_UINT16]);
    return res;
}

UA_StatusCode
initNS0LogObject(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Remove the LogObject nodes when the feature is disabled at runtime. The
     * type nodes remain. */
    if(!server->config.logObjectsEnabled) {
        deleteNode(server, UA_NS0ID(SERVERLOG), true);
        deleteNode(server, UA_NS0ID(LOGS), true);
        deleteNode(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS),
                   true);
#ifndef UA_GENERATED_NAMESPACE_ZERO_FULL
        /* The Resources Folder of Part 22 only carries the Logs Folder in
         * the reduced namespace zero */
        deleteNode(server, UA_NS0ID(RESOURCES), true);
#endif
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* The optional ReleaseContinuationPoint Method of the LogObjectType is
     * not instantiated on the standard ServerLog. Reference the Method of the
     * type, as done for the Methods of instantiated Objects. */
    res |= addRef(server, UA_NS0ID(SERVERLOG), UA_NS0ID(HASCOMPONENT),
                  UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT), true);

    /* Mirror the configured limits into the Properties of the ServerLog */
    UA_LogObjectSettings *s = &server->config.serverLog;
    res |= writeLogObjectProperty(server, UA_NS0ID_SERVERLOG_MAXRECORDS,
                                  &s->maxRecords, &UA_TYPES[UA_TYPES_UINT32]);
    res |= writeLogObjectProperty(server, UA_NS0ID_SERVERLOG_MINIMUMSEVERITY,
                                  &s->minimumSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    if(s->maxStorageDuration > 0.0) {
        res |= writeLogObjectProperty(server, UA_NS0ID_SERVERLOG_MAXSTORAGEDURATION,
                                      &s->maxStorageDuration,
                                      &UA_TYPES[UA_TYPES_DURATION]);
    } else {
        /* Zero is not a valid MaxStorageDuration (Part 26, 5.2). Without a
         * time-based limit the optional Property is not exposed. */
        deleteNode(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), true);
    }

    res |= addMaxContinuationPointsProperty(server);
    return res;
}

#endif /* UA_ENABLE_LOGOBJECT */
