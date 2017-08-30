/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"

/*****************/
/* Node Creation */
/*****************/

static UA_StatusCode
addReferenceInternal(UA_Server *server, UA_UInt32 sourceId, UA_UInt32 refTypeId,
                     UA_UInt32 targetId, UA_Boolean isForward) {
    return UA_Server_addReference(server, UA_NODEID_NUMERIC(0, sourceId),
                           UA_NODEID_NUMERIC(0, refTypeId),
                           UA_EXPANDEDNODEID_NUMERIC(0, targetId), isForward);
}


static UA_StatusCode
addReferenceTypeNode(UA_Server *server, char* name, char *inverseName, UA_UInt32 referencetypeid,
                     UA_Boolean isabstract, UA_Boolean symmetric, UA_UInt32 parentid) {
    UA_ReferenceTypeAttributes reference_attr = UA_ReferenceTypeAttributes_default;
    reference_attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    reference_attr.isAbstract = isabstract;
    reference_attr.symmetric = symmetric;
    if(inverseName)
        reference_attr.inverseName = UA_LOCALIZEDTEXT("en_US", inverseName);
    return UA_Server_addReferenceTypeNode(server, UA_NODEID_NUMERIC(0, referencetypeid),
                                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                   UA_QUALIFIEDNAME(0, name), reference_attr, NULL, NULL);
}


static UA_StatusCode
addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid,
                  UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.isAbstract = isAbstract;
    return UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(0, objecttypeid),
                                UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}


static UA_StatusCode
addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid,
                UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.isAbstract = isAbstract;
    return UA_Server_addDataTypeNode(server, UA_NODEID_NUMERIC(0, datatypeid),
                              UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                              UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}

static UA_StatusCode
addVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                    UA_Boolean isAbstract, UA_Int32 valueRank, UA_UInt32 dataType,
                    const UA_DataType *type, UA_UInt32 parentid) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.dataType = UA_NODEID_NUMERIC(0, dataType);
    attr.isAbstract = isAbstract;
    attr.valueRank = valueRank;
    if(type) {
        void *val = UA_alloca(type->memSize);
        UA_init(val, type);
        UA_Variant_setScalar(&attr.value, val, type);
    }
    return UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, variabletypeid),
                                  UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                  UA_QUALIFIEDNAME(0, name), UA_NODEID_NULL, attr, NULL, NULL);
}


static UA_StatusCode
addObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 type_id) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, objectid),
                            UA_NODEID_NUMERIC(0, parentid),
                            UA_NODEID_NUMERIC(0, referenceid),
                            UA_QUALIFIEDNAME(0, name),
                            UA_NODEID_NUMERIC(0, type_id),
                            object_attr, NULL, NULL);

}


/**********************/
/* Create Namespace 0 */
/**********************/

void UA_Server_createNS0_base(UA_Server *server) {
    
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    /*********************************/
    /* Bootstrap reference hierarchy */
    /*********************************/

    /* Bootstrap References and HasSubtype */
    UA_ReferenceTypeAttributes references_attr = UA_ReferenceTypeAttributes_default;
    references_attr.displayName = UA_LOCALIZEDTEXT("en_US", "References");
    references_attr.isAbstract = true;
    references_attr.symmetric = true;
    references_attr.inverseName = UA_LOCALIZEDTEXT("en_US", "References");
    ret |= UA_Server_addReferenceTypeNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES),
                                         UA_QUALIFIEDNAME(0, "References"), references_attr, NULL, NULL);
    
    UA_ReferenceTypeAttributes hassubtype_attr = UA_ReferenceTypeAttributes_default;
    hassubtype_attr.displayName = UA_LOCALIZEDTEXT("en_US", "HasSubtype");
    hassubtype_attr.isAbstract = false;
    hassubtype_attr.symmetric = false;
    hassubtype_attr.inverseName = UA_LOCALIZEDTEXT("en_US", "SubtypeOf");
    ret |= UA_Server_addReferenceTypeNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(0, "HasSubtype"), hassubtype_attr, NULL, NULL);

    ret |= addReferenceTypeNode(server, "HierarchicalReferences", NULL,
                         UA_NS0ID_HIERARCHICALREFERENCES,
                         true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "NonHierarchicalReferences", NULL,
                         UA_NS0ID_NONHIERARCHICALREFERENCES,
                         true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "HasChild", NULL, UA_NS0ID_HASCHILD,
                         true, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "Organizes", "OrganizedBy", UA_NS0ID_ORGANIZES,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEventSource", "EventSourceOf", UA_NS0ID_HASEVENTSOURCE,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasModellingRule", "ModellingRuleOf", UA_NS0ID_HASMODELLINGRULE,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEncoding", "EncodingOf", UA_NS0ID_HASENCODING,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasDescription", "DescriptionOf", UA_NS0ID_HASDESCRIPTION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasTypeDefinition", "TypeDefinitionOf", UA_NS0ID_HASTYPEDEFINITION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "GeneratesEvent", "GeneratedBy", UA_NS0ID_GENERATESEVENT,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "Aggregates", "AggregatedBy", UA_NS0ID_AGGREGATES,
                         false, false, UA_NS0ID_HASCHILD);

    ret |= addReferenceInternal(server, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE,
                         UA_NS0ID_HASSUBTYPE, true);

    ret |= addReferenceTypeNode(server, "HasProperty", "PropertyOf", UA_NS0ID_HASPROPERTY,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasComponent", "ComponentOf", UA_NS0ID_HASCOMPONENT,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasNotifier", "NotifierOf", UA_NS0ID_HASNOTIFIER,
                         false, false, UA_NS0ID_HASEVENTSOURCE);

    ret |= addReferenceTypeNode(server, "HasOrderedComponent", "OrderedComponentOf",
                         UA_NS0ID_HASORDEREDCOMPONENT, false, false, UA_NS0ID_HASCOMPONENT);

    /***************/
    /* ObjectTypes */
    /***************/

    /* Bootstrap BaseObjectType */
    UA_ObjectTypeAttributes baseobj_attr = UA_ObjectTypeAttributes_default;
    baseobj_attr.displayName = UA_LOCALIZEDTEXT("en_US", "BaseObjectType");
    ret |= UA_Server_addObjectTypeNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                      UA_QUALIFIEDNAME(0, "BaseObjectType"), baseobj_attr, NULL, NULL);

    ret |= addObjectTypeNode(server, "FolderType", UA_NS0ID_FOLDERTYPE,
                      false, UA_NS0ID_BASEOBJECTTYPE);

    /*ret |= addObjectTypeNode(server, "ModellingRuleType", UA_NS0ID_MODELLINGRULETYPE,
                      false, UA_NS0ID_BASEOBJECTTYPE);


    ret |= addObjectTypeNode(server, "ServerType", UA_NS0ID_SERVERTYPE,
                      false, UA_NS0ID_BASEOBJECTTYPE);

    ret |= addObjectTypeNode(server, "ServerDiagnosticsType", UA_NS0ID_SERVERDIAGNOSTICSTYPE,
                      false,  UA_NS0ID_BASEOBJECTTYPE);

    ret |= addObjectTypeNode(server, "ServerCapatilitiesType", UA_NS0ID_SERVERCAPABILITIESTYPE,
                      false, UA_NS0ID_BASEOBJECTTYPE);*/


    /**************/
    /* Data Types */
    /**************/

    /* Bootstrap BaseDataType */
    UA_DataTypeAttributes basedatatype_attr = UA_DataTypeAttributes_default;
    basedatatype_attr.displayName = UA_LOCALIZEDTEXT("en_US", "BaseDataType");
    basedatatype_attr.isAbstract = true;
    ret |= UA_Server_addDataTypeNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE),
                                    UA_QUALIFIEDNAME(0, "BaseDataType"), basedatatype_attr, NULL, NULL);

    ret |= addDataTypeNode(server, "Number", UA_NS0ID_NUMBER, true, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Integer", UA_NS0ID_INTEGER, true, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "UInteger", UA_NS0ID_UINTEGER, true, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "Boolean", UA_NS0ID_BOOLEAN, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "SByte", UA_NS0ID_SBYTE, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "Byte", UA_NS0ID_BYTE, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int16", UA_NS0ID_INT16, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt16", UA_NS0ID_UINT16, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int32", UA_NS0ID_INT32, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt32", UA_NS0ID_UINT32, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int64", UA_NS0ID_INT64, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt64", UA_NS0ID_UINT64, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Float", UA_NS0ID_FLOAT, false, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "Double", UA_NS0ID_DOUBLE, false, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "DateTime", UA_NS0ID_DATETIME, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "String", UA_NS0ID_STRING, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "ByteString", UA_NS0ID_BYTESTRING, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Guid", UA_NS0ID_GUID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "XmlElement", UA_NS0ID_XMLELEMENT, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "NodeId", UA_NS0ID_NODEID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "ExpandedNodeId", UA_NS0ID_EXPANDEDNODEID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "QualifiedName", UA_NS0ID_QUALIFIEDNAME, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "LocalizedText", UA_NS0ID_LOCALIZEDTEXT, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "StatusCode", UA_NS0ID_STATUSCODE, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Structure", UA_NS0ID_STRUCTURE, true, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Decimal128", UA_NS0ID_DECIMAL128, false, UA_NS0ID_NUMBER);

    ret |= addDataTypeNode(server, "Duration", UA_NS0ID_DURATION, false, UA_NS0ID_DOUBLE);
    ret |= addDataTypeNode(server, "UtcTime", UA_NS0ID_UTCTIME, false, UA_NS0ID_DATETIME);
    ret |= addDataTypeNode(server, "LocaleId", UA_NS0ID_LOCALEID, false, UA_NS0ID_STRING);

//    ret |= addDataTypeNode(server, "ServerStatusDataType", UA_NS0ID_SERVERSTATUSDATATYPE, false, UA_NS0ID_STRUCTURE);
//    ret |= addDataTypeNode(server, "BuildInfo", UA_NS0ID_BUILDINFO, false, UA_NS0ID_STRUCTURE);
//    ret |= addDataTypeNode(server, "DataValue", UA_NS0ID_DATAVALUE, false, UA_NS0ID_BASEDATATYPE);
//    ret |= addDataTypeNode(server, "DiagnosticInfo", UA_NS0ID_DIAGNOSTICINFO, false, UA_NS0ID_BASEDATATYPE);
//    ret |= addDataTypeNode(server, "Enumeration", UA_NS0ID_ENUMERATION, true, UA_NS0ID_BASEDATATYPE);
//    ret |= addDataTypeNode(server, "ServerState", UA_NS0ID_SERVERSTATE, false, UA_NS0ID_ENUMERATION);

    /*****************/
    /* VariableTypes */
    /*****************/

    /* Bootstrap BaseVariableType */
    UA_VariableTypeAttributes basevar_attr = UA_VariableTypeAttributes_default;
    basevar_attr.displayName = UA_LOCALIZEDTEXT("en_US", "BaseVariableType");
    basevar_attr.isAbstract = true;
    basevar_attr.valueRank = -2;
    basevar_attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ret |= UA_Server_addVariableTypeNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                                        UA_QUALIFIEDNAME(0, "BaseVariableType"), basevar_attr, NULL, NULL);

    ret |= addVariableTypeNode(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE,
                        false, -2, UA_NS0ID_BASEDATATYPE, NULL, UA_NS0ID_BASEVARIABLETYPE);

    /*ret |= addVariableTypeNode(server, "PropertyType", UA_NS0ID_PROPERTYTYPE,
                        false, -2, UA_NS0ID_BASEDATATYPE, NULL, UA_NS0ID_BASEVARIABLETYPE);

    ret |= addVariableTypeNode(server, "BuildInfoType", UA_NS0ID_BUILDINFOTYPE,
                        false, -1, UA_NS0ID_BUILDINFO, &UA_TYPES[UA_TYPES_BUILDINFO],
                        UA_NS0ID_BASEDATAVARIABLETYPE);

    ret |= addVariableTypeNode(server, "ServerStatusType", UA_NS0ID_SERVERSTATUSTYPE,
                        false, -1, UA_NS0ID_SERVERSTATUSDATATYPE, &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE],
                        UA_NS0ID_BASEDATAVARIABLETYPE);
*/

    /******************/
    /* Root and below */
    /******************/

    UA_ObjectAttributes root_attr = UA_ObjectAttributes_default;
    root_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Root");
    ret |= UA_Server_addObjectNode_begin(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                                  UA_QUALIFIEDNAME(0, "Root"), root_attr, NULL, NULL);
    ret |= addReferenceInternal(server, UA_NS0ID_ROOTFOLDER, UA_NS0ID_HASTYPEDEFINITION,
                         UA_NS0ID_FOLDERTYPE, true);

    ret |= addObjectNode(server, "Objects", UA_NS0ID_OBJECTSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Types", UA_NS0ID_TYPESFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "ReferenceTypes", UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addReferenceInternal(server, UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_ORGANIZES,
                         UA_NS0ID_REFERENCES, true);

    ret |= addObjectNode(server, "DataTypes", UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addReferenceInternal(server, UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_ORGANIZES,
                         UA_NS0ID_BASEDATATYPE, true);

    ret |= addObjectNode(server, "VariableTypes", UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addReferenceInternal(server, UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_ORGANIZES,
                         UA_NS0ID_BASEVARIABLETYPE, true);

    ret |= addObjectNode(server, "ObjectTypes", UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addReferenceInternal(server, UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_ORGANIZES,
                         UA_NS0ID_BASEOBJECTTYPE, true);

    ret |= addObjectNode(server, "EventTypes", UA_NS0ID_EVENTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Views", UA_NS0ID_VIEWSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    assert(ret == UA_STATUSCODE_GOOD);
}
