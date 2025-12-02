/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"

/****************************/
/* Custom DataType Handling */
/****************************/

const UA_DataTypeArray *
serverCustomTypes(UA_Server *server) {
    if(server->customTypes_internalSize == 0)
        return server->config.customDataTypes;
    server->customTypes_internal[server->customTypes_internalSize-1].next = server->config.customDataTypes;
    return server->customTypes_internal;
}

const UA_DataTypeArray *
UA_Server_getDataTypes(UA_Server *server) {
    lockServer(server);
    const UA_DataTypeArray * out = serverCustomTypes(server);
    unlockServer(server);
    return out;
}

const UA_DataType *
UA_Server_findDataType(UA_Server *server, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, serverCustomTypes(server));
}

/* DataTypes need a stable pointer. So we allocate an array of 64 datatypes.
 * When that is full we move the server->customTypes_internal into a
 * heap-structure that gets cleaned up with the server lifecycle. */
static UA_StatusCode
addDataType(UA_Server *server, UA_DataType *dt) {
    /* Allocate the space for the new DataType */
#define TYPES_LIST_SIZE 64
    UA_DataTypeArray *current = NULL;
    if(server->customTypes_internalSize > 0)
        current = &server->customTypes_internal[server->customTypes_internalSize-1];
    if(!current || current->typesSize == TYPES_LIST_SIZE) {
        /* Increase the list-of-lists size */
        UA_DataTypeArray *lol = (UA_DataTypeArray*)
            UA_realloc(server->customTypes_internal,
                       sizeof(UA_DataTypeArray) * (server->customTypes_internalSize+1));
        if(!lol)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memset(&lol[server->customTypes_internalSize], 0, sizeof(UA_DataTypeArray));
        server->customTypes_internal = lol;
        server->customTypes_internalSize++;

        /* Update the next-pointers for the internal DataTypeArray */
        for(size_t i = 0; i < server->customTypes_internalSize-1; i++)
            lol[i].next = &lol[i+1];

        /* Add a new types list. With the space for the datatypes already appended */
        current = &server->customTypes_internal[server->customTypes_internalSize-1];
        current->types = (UA_DataType*)UA_calloc(TYPES_LIST_SIZE, sizeof(UA_DataType));
        current->typesSize = 0;
        if(!current->types) {
            server->customTypes_internalSize--;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    /* Move the datatype into the stable location in the server */
    current->types[current->typesSize] = *dt;
    current->typesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addDataType(UA_Server *server, const UA_NodeId parentNodeId,
                      const UA_DataType *type) {
    /* Check that the type does not already exist. We do not allow changes to
     * DataTypes once they are set. */
    if(UA_Server_findDataType(server, &type->typeId))
        return UA_STATUSCODE_BADNODEIDEXISTS;

    /* Make a copy of the UA_DataType */
    UA_DataType dt2;
    UA_StatusCode res = UA_DataType_copy(type, &dt2);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Add the UA_DataType to the server */
    res = addDataType(server, &dt2);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataType_clear(&dt2);
    return res;
}

UA_StatusCode
UA_Server_addDataTypeFromDescription(UA_Server *server,
                                     const UA_ExtensionObject *description) {
    /* Translate into a new UA_DataType */
    UA_DataType dt;
    UA_StatusCode res =
        UA_DataType_fromDescription(&dt, description, serverCustomTypes(server));
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Check that the type does not already exist. We do not allow changes to
     * DataTypes once they are set. */
    if(UA_Server_findDataType(server, &dt.typeId)) {
        UA_DataType_clear(&dt);
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    /* Add the UA_DataType to the server */
    res = addDataType(server, &dt);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataType_clear(&dt);
    return res;
}

/********************************/
/* Information Model Operations */
/********************************/

struct ReturnTypeContext {
    UA_Server *server;
    UA_UInt32 attributeMask;
    UA_ReferenceTypeSet references;
    UA_BrowseDirection referenceDirections;
};

static void *
returnFirstType(void *context, UA_ReferenceTarget *t) {
    struct ReturnTypeContext *ctx = (struct ReturnTypeContext*)context;
    /* Don't release the node that is returned.
     * Continues to iterate if NULL is returned. */
    return (void *)(uintptr_t)UA_NODESTORE_GETFROMREF_SELECTIVE(
        ctx->server, t->targetId, ctx->attributeMask, ctx->references,
        ctx->referenceDirections);
}

const UA_Node *
getNodeType(UA_Server *server, const UA_NodeHead *head,
            UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
            UA_BrowseDirection referenceDirections) {
    /* The reference to the parent is different for variable and variabletype */
    UA_Byte parentRefIndex;
    UA_Boolean inverse;
    switch(head->nodeClass) {
    case UA_NODECLASS_OBJECT:
    case UA_NODECLASS_VARIABLE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASTYPEDEFINITION;
        inverse = false;
        break;
    case UA_NODECLASS_OBJECTTYPE:
    case UA_NODECLASS_VARIABLETYPE:
    case UA_NODECLASS_REFERENCETYPE:
    case UA_NODECLASS_DATATYPE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASSUBTYPE;
        inverse = true;
        break;
    default:
        return NULL;
    }

    struct ReturnTypeContext ctx;
    ctx.server = server;
    ctx.attributeMask = attributeMask;
    ctx.references = references;
    ctx.referenceDirections = referenceDirections;

    /* Return the first matching candidate */
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse != inverse)
            continue;
        if(rk->referenceTypeIndex != parentRefIndex)
            continue;
        const UA_Node *type = (const UA_Node*)
            UA_NodeReferenceKind_iterate(rk, returnFirstType, &ctx);
        if(type)
            return type;
    }

    return NULL;
}

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_NodeHead *head) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        if(head->references[i].isInverse == false &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASSUBTYPE)
            return true;
        if(head->references[i].isInverse == true &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASTYPEDEFINITION)
            return true;
    }
    return false;
}

UA_StatusCode
getTypeAndInterfaceHierarchy(UA_Server *server, const UA_NodeId *leafNode,
                             UA_Boolean includeLeaf, UA_NodeId **typeHierarchy,
                             size_t *typeHierarchySize) {
    UA_ReferenceTypeSet hastype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    UA_ReferenceTypeSet hassubtype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ReferenceTypeSet hasinterface = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);

    /* Initialize the tree and add the leaf */
    RefTree rt;
    UA_StatusCode res = RefTree_init(&rt);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = RefTree_addNodeId(&rt, leafNode, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hastype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all super types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_INVERSE, &hassubtype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all interfaces */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hasinterface,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);

 errout:
    if(res != UA_STATUSCODE_GOOD || rt.size == 0) {
        RefTree_clear(&rt);
        return res;
    }

    /* Make the array of ExpandedNodeId into an array of NodeId */
    UA_NodeId *outArray = (UA_NodeId*)rt.targets;
    size_t pos = 0;
    for(size_t i = 0; i < rt.size; i++) {
        UA_NodeId *n = &outArray[pos];
        UA_ExpandedNodeId *e = &rt.targets[i];
        if(!UA_ExpandedNodeId_isLocal(e)) {
            UA_ExpandedNodeId_clear(e);
            continue;
        }
        *n = e->nodeId;
        UA_String_clear(&e->namespaceUri);
        pos++;
    }

    *typeHierarchySize = pos;
    *typeHierarchy = outArray;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
getAllInterfaces(UA_Server *server, const UA_NodeId *objectNode,
                 UA_NodeId **interfaceNodes, size_t *interfaceNodesSize) {
    UA_ReferenceTypeSet hastype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    UA_ReferenceTypeSet hassubtype = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ReferenceTypeSet hasinterface = UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);

    /* Initialize the tree and add the leaf */
    size_t beforeInterfaces = 0;
    RefTree rt;
    UA_StatusCode res = RefTree_init(&rt);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = RefTree_addNodeId(&rt, objectNode, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hastype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all super types */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_INVERSE, &hassubtype,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Get all interfaces */
    beforeInterfaces = rt.size; /* Return only the interfaces */
    res = browseRecursiveRefTree(server, &rt, UA_BROWSEDIRECTION_FORWARD, &hasinterface,
                                 UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);

 errout:
    if(res != UA_STATUSCODE_GOOD || rt.size == 0) {
        RefTree_clear(&rt);
        return res;
    }

    /* Make the array of ExpandedNodeId into an array of NodeId */
    UA_NodeId *outArray = (UA_NodeId*)rt.targets;
    size_t pos = 0;
    for(size_t i = 0; i < rt.size; i++) {
        UA_NodeId *n = &outArray[pos];
        UA_ExpandedNodeId *e = &rt.targets[i];
        if(i < beforeInterfaces || !UA_ExpandedNodeId_isLocal(e)) {
            UA_ExpandedNodeId_clear(e);
            continue;
        }
        *n = e->nodeId;
        UA_String_clear(&e->namespaceUri);
        pos++;
    }

    /* No interfaces found */
    if(pos == 0) {
        RefTree_clear(&rt);
        outArray = NULL;
    }

    *interfaceNodesSize = pos;
    *interfaceNodes = outArray;
    return UA_STATUSCODE_GOOD;
}

/* Get the node, make the changes and release */
UA_StatusCode
editNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
         UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
         UA_BrowseDirection referenceDirections,
         UA_EditNodeCallback callback, void *data) {
    UA_Node *node =
        UA_NODESTORE_GET_EDIT_SELECTIVE(server, nodeId, attributeMask,
                                        references, referenceDirections);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode retval = callback(server, session, node, data);
    UA_NODESTORE_RELEASE(server, node);
    return retval;
}

/**************************/
/* Certificate Validation */
/**************************/

UA_StatusCode
validateCertificate(UA_Server *server, UA_CertificateGroup *cg,
                    UA_SecureChannel *channel, UA_Session *session,
                    const char *logPrefix,
                    const UA_ApplicationDescription *ad,
                    const UA_ByteString certificate) {
    /* Verify the ApplicationUri */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ad) {
        res = UA_CertificateUtils_verifyApplicationUri(&certificate,
                                                       &ad->applicationUri);
        if(res != UA_STATUSCODE_GOOD) {
            if(server->config.allowAllCertificateUris <= UA_RULEHANDLING_WARN) {
                if(session) {
                    UA_LOG_ERROR_SESSION(server->config.logging, session,
                                         "%s: The client's ApplicationUri "
                                         "could not be verified against the "
                                         "ApplicationUri %S from the client's "
                                         "ApplicationDescription", logPrefix,
                                         ad->applicationUri);
                } else if(channel) {
                    UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                         "%s: The client certificate's ApplicationUri "
                                         "could not be verified against the "
                                         "ApplicationUri %S from the client's "
                                         "ApplicationDescription", logPrefix,
                                         ad->applicationUri);
                } else {
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "%s: The server certificate's ApplicationUri "
                                 "could not be verified against the "
                                 "ApplicationUri %S from its "
                                 "ApplicationDescription", logPrefix,
                                 ad->applicationUri);
                }
            }
            if(server->config.allowAllCertificateUris <= UA_RULEHANDLING_ABORT)
                return UA_STATUSCODE_BADCERTIFICATEINVALID;
        }
    }

    if(!cg->verifyCertificate) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "%s: Could not validate the certificate "
                     "as the CertificateGroup is not configured", logPrefix);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Validate in the CertificateGroup */
    res = cg->verifyCertificate(cg, &certificate);
    if(res != UA_STATUSCODE_GOOD) {
        const char *descr = UA_StatusCode_name(res);
        if(session) {
            UA_LOG_ERROR_SESSION(server->config.logging, session,
                                 "%s: The client certificate failed the verification "
                                 "in the CertificateGroup with StatusCode %s",
                                 logPrefix, descr);
        } else if(channel) {
            UA_LOG_ERROR_CHANNEL(server->config.logging, channel,
                                 "%s: The client certificate failed the verification "
                                 "in the CertificateGroup with StatusCode %s",
                                 logPrefix, descr);
        } else {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "%s: The client certificate failed the verification "
                         "in the CertificateGroup with StatusCode %s",
                         logPrefix, descr);
        }
    }
    return res;
}

/************/
/* Auditing */
/************/

#ifdef UA_ENABLE_AUDITING

void
auditEvent(UA_Server *server, UA_ApplicationNotificationType type,
           UA_SecureChannel *channel, UA_Session *session, const char *serviceName,
           UA_Boolean status, const UA_KeyValueMap payload) {
    UA_ServerConfig *config = &server->config;

    /* Set the values for AuditEventType fields:
     * /ActionTimeStamp    -> 0
     * /Status             -> 1
     * /ServerId           -> 2
     * /ClientAuditEntryId -> 3
     * /ClientUserId       -> 4 */

    UA_NodeId sessionId = (session) ? session->sessionId: UA_NODEID_NULL;
    UA_Byte entryIdBuf[521];
    UA_String auditEntryId = {512, entryIdBuf};
    UA_String_format(&auditEntryId, "%lu:%N:%s",
                     (long unsigned)channel->securityToken.channelId,
                     sessionId, serviceName);
    UA_String clientUserId = (session) ?
        session->clientUserIdOfSession : UA_STRING_NULL;
    UA_DateTime actionTimestamp =
        config->eventLoop->dateTime_now(config->eventLoop);

    UA_Variant_setScalar(&payload.map[0].value, &actionTimestamp,
                         &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&payload.map[1].value, &status,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&payload.map[2].value,
                         &config->applicationDescription.applicationUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&payload.map[3].value, &auditEntryId,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&payload.map[4].value, &clientUserId,
                         &UA_TYPES[UA_TYPES_STRING]);

    /* Call the server notification callback */
    if(config->auditNotificationCallback)
        config->auditNotificationCallback(server, type, payload);
    if(config->globalNotificationCallback)
        config->globalNotificationCallback(server, type, payload);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    /* Create the Event in the information model */
    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventFields = &payload;
    switch(type) {
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT:
        ed.eventType = UA_NS0ID(AUDITEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY:
        ed.eventType = UA_NS0ID(AUDITSECURITYEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL:
        ed.eventType = UA_NS0ID(AUDITCHANNELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN:
        ed.eventType = UA_NS0ID(AUDITOPENSECURECHANNELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION:
        ed.eventType = UA_NS0ID(AUDITSESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CREATE:
        ed.eventType = UA_NS0ID(AUDITCREATESESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE:
        ed.eventType = UA_NS0ID(AUDITACTIVATESESSIONEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CANCEL:
        ed.eventType = UA_NS0ID(AUDITCANCELEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_DATAMISMATCH:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEDATAMISMATCHEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_EXPIRED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEEXPIREDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_INVALID:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEINVALIDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_UNTRUSTED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEUNTRUSTEDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_REVOKED:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEREVOKEDEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CERTIFICATE_MISMATCH:
        ed.eventType = UA_NS0ID(AUDITCERTIFICATEMISMATCHEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE:
        ed.eventType = UA_NS0ID(AUDITNODEMANAGEMENTEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADD:
        ed.eventType = UA_NS0ID(AUDITADDNODESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETE:
        ed.eventType = UA_NS0ID(AUDITDELETENODESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_ADDREFERENCES:
        ed.eventType = UA_NS0ID(AUDITADDREFERENCESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_NODE_DELETEREFERENCES:
        ed.eventType = UA_NS0ID(AUDITDELETEREFERENCESEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE:
        ed.eventType = UA_NS0ID(AUDITUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_WRITE:
        ed.eventType = UA_NS0ID(AUDITWRITEUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_HISTORY:
        ed.eventType = UA_NS0ID(AUDITHISTORYUPDATEEVENTTYPE); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD:
        ed.eventType = UA_NS0ID(AUDITUPDATEMETHODEVENTTYPE); break;
    default:
        /* TODO:
         * UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_CLIENT                            = 0x1800,
         * UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_CLIENT_UPDATEMETHOD               = 0x1810
        */
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Unsupported audit log type requested: %u", type);
        return;
    }
    createEvent(server, &ed, NULL);
 #endif
}

void
auditSecurityEvent(UA_Server *server, UA_ApplicationNotificationType type,
                   UA_SecureChannel *channel, UA_Session *session,
                   const char *serviceName, UA_Boolean status, UA_StatusCode statusCodeId,
                   const UA_KeyValueMap payload) {
    UA_Variant_setScalar(&payload.map[5].value, &statusCodeId,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
    auditEvent(server, type, channel, session, serviceName, status, payload);
}

void
auditChannelEvent(UA_Server *server, UA_ApplicationNotificationType type,
                  UA_SecureChannel *channel, UA_Session *session, const char *serviceName,
                  UA_Boolean status, UA_StatusCode statusCodeId,
                  const UA_KeyValueMap payload) {
    UA_Byte secureChannelNameBuf[32];
    UA_String secureChannelName = {32, secureChannelNameBuf};
    UA_String_format(&secureChannelName, "%lu",
                     (long unsigned)channel->securityToken.channelId);
    UA_Byte sourceNameBuf[128];
    UA_String sourceName = {128, sourceNameBuf};
    UA_String_format(&sourceName, "SecureChannel/%s", serviceName);
    UA_Variant_setScalar(&payload.map[6].value, &secureChannelName,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&payload.map[7].value, &sourceName,
                         &UA_TYPES[UA_TYPES_STRING]);
    auditSecurityEvent(server, type, channel, session, serviceName, status,
                       statusCodeId, payload);
}

#endif /* UA_ENABLE_AUDITING */

/*********************************/
/* Default attribute definitions */
/*********************************/

const UA_ObjectAttributes UA_ObjectAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    0                       /* eventNotifier */
};

const UA_VariableAttributes UA_VariableAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    UA_ACCESSLEVELMASK_READ |    /* accessLevel */
    UA_ACCESSLEVELMASK_STATUSWRITE |
    UA_ACCESSLEVELMASK_TIMESTAMPWRITE,
    0,                           /* userAccessLevel */
    0.0,                         /* minimumSamplingInterval */
    false                        /* historizing */
};

const UA_MethodAttributes UA_MethodAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    true, true              /* executable (userExecutable) */
};

const UA_ObjectTypeAttributes UA_ObjectTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_VariableTypeAttributes UA_VariableTypeAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    false                        /* isAbstract */
};

const UA_ReferenceTypeAttributes UA_ReferenceTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* isAbstract */
    false,                  /* symmetric */
    {{0, NULL}, {0, NULL}}  /* inverseName */
};

const UA_DataTypeAttributes UA_DataTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_ViewAttributes UA_ViewAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* containsNoLoops */
    0                       /* eventNotifier */
};

