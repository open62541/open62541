/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/types.h>
#include <open62541/util.h>
#include "util/ua_util_internal.h"

/*************************/
/* Generic Functionality */
/*************************/

/* All descriptions begin with UA_DataTypeDescription */

static UA_StatusCode
fromDescription(UA_DataType *type, const UA_DataTypeDescription *descr) {
    memset(type, 0, sizeof(UA_DataType));
    UA_StatusCode res = UA_NodeId_copy(&descr->dataTypeId, &type->typeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_Byte bufChars[512];
    UA_ByteString buf = {512, bufChars};
    res = UA_QualifiedName_print(&descr->name, &buf);
    UA_CHECK_STATUS(res, UA_DataType_clear(type); return res);

    type->typeName = (char*)UA_malloc(buf.length + 1);
    UA_CHECK(type->typeName != 0,
             UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
    memcpy((void*)(uintptr_t)type->typeName, buf.data, buf.length);
    *(char*)(uintptr_t)&type->typeName[buf.length] = '\0';
#endif

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
toDescription(const UA_DataType *type, UA_DataTypeDescription *descr) {
    /* Parse the typeName into the BrowseName (QualifiedName) */
    UA_StatusCode res;
#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_String nameStr = UA_STRING((char*)(uintptr_t)type->typeName);
    res = UA_QualifiedName_parse(&descr->name, nameStr);
    if(res != UA_STATUSCODE_GOOD)
        return res;
#endif

    /* Copy the NodeId */
    res = UA_NodeId_copy(&type->typeId, &descr->dataTypeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_QualifiedName_clear(&descr->name);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

/******************/
/* Structure Type */
/******************/

/* Compute padding of structure elements based on the alignment requirements
 * of the builtin datatypes */

#define PADSTRUCT(T) struct _pad_##T {char _; T x;};
#define ALIGNMENT(T) offsetof(struct _pad_##T, x)
#define PADDING(MEMSIZE, ALIGN) (ALIGN - (MEMSIZE % ALIGN)) % ALIGN

PADSTRUCT(UA_Boolean)
PADSTRUCT(UA_SByte)
PADSTRUCT(UA_Byte)
PADSTRUCT(UA_Int16)
PADSTRUCT(UA_UInt16)
PADSTRUCT(UA_Int32)
PADSTRUCT(UA_UInt32)
PADSTRUCT(UA_Int64)
PADSTRUCT(UA_UInt64)
PADSTRUCT(UA_Float)
PADSTRUCT(UA_Double)
PADSTRUCT(UA_String)
PADSTRUCT(UA_DateTime)
PADSTRUCT(UA_Guid)
PADSTRUCT(UA_ByteString)
PADSTRUCT(UA_XmlElement)
PADSTRUCT(UA_NodeId)
PADSTRUCT(UA_ExpandedNodeId)
PADSTRUCT(UA_StatusCode)
PADSTRUCT(UA_QualifiedName)
PADSTRUCT(UA_LocalizedText)
PADSTRUCT(UA_ExtensionObject)
PADSTRUCT(UA_DataValue)
PADSTRUCT(UA_Variant)
PADSTRUCT(UA_DiagnosticInfo)

PADSTRUCT(uintptr_t)
PADSTRUCT(size_t)

/* TODO: Decimal and BitfieldCluster are not implemented in the SDK so far */
static UA_Byte alignment[UA_DATATYPEKINDS] = {
    ALIGNMENT(UA_Boolean), ALIGNMENT(UA_SByte), ALIGNMENT(UA_Byte), ALIGNMENT(UA_Int16),
    ALIGNMENT(UA_UInt16), ALIGNMENT(UA_Int32), ALIGNMENT(UA_UInt32), ALIGNMENT(UA_Int64),
    ALIGNMENT(UA_UInt64), ALIGNMENT(UA_Float), ALIGNMENT(UA_Double), ALIGNMENT(UA_String),
    ALIGNMENT(UA_DateTime), ALIGNMENT(UA_Guid), ALIGNMENT(UA_ByteString),
    ALIGNMENT(UA_XmlElement), ALIGNMENT(UA_NodeId), ALIGNMENT(UA_ExpandedNodeId),
    ALIGNMENT(UA_StatusCode), ALIGNMENT(UA_QualifiedName), ALIGNMENT(UA_LocalizedText),
    ALIGNMENT(UA_ExtensionObject), ALIGNMENT(UA_DataValue), ALIGNMENT(UA_Variant),
    ALIGNMENT(UA_DiagnosticInfo), 0, ALIGNMENT(UA_UInt32), 0, 0, 0, 0
};

static UA_Byte
type_alignment(const UA_DataType *type) {
    if(type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
       type->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
       type->typeKind == UA_DATATYPEKIND_UNION) {
        /* Maximum alignment of any member */
        UA_Byte align = 1;
        for(size_t i = 0; i < type->membersSize; i++) {
            const UA_DataTypeMember *dtm = &type->members[i];
            if(dtm->isArray) {
                UA_Byte sizeAlign = offsetof(struct _pad_size_t, x);
                if(sizeAlign> align)
                    align = sizeAlign;
            }
            if(dtm->isArray || dtm->isOptional) {
                UA_Byte ptrAlign = offsetof(struct _pad_uintptr_t, x);
                if(ptrAlign> align)
                    align = ptrAlign;
                continue;
            }
            UA_Byte memberAlign = type_alignment(type->members[i].memberType);
            if(memberAlign > align)
                align = memberAlign;
        }
        return align;
    }
    return alignment[type->typeKind];
}

static UA_StatusCode
UA_DataType_fromStructureDescription(UA_DataType *type,
                                     const UA_StructureDescription *descr,
                                     const UA_DataTypeArray *customTypes) {
    /* Set the basic description */
    UA_StatusCode res = fromDescription(type, (const UA_DataTypeDescription*)descr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the typeKind */
    const UA_StructureDefinition *sd = &descr->structureDefinition;
    switch(sd->structureType) {
    case UA_STRUCTURETYPE_STRUCTURE:
        type->typeKind = UA_DATATYPEKIND_STRUCTURE; break;
    case UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS:
        type->typeKind = UA_DATATYPEKIND_OPTSTRUCT; break;
    case UA_STRUCTURETYPE_UNION:
        type->typeKind = UA_DATATYPEKIND_UNION; break;
    case UA_STRUCTURETYPE_STRUCTUREWITHSUBTYPEDVALUES:
    case UA_STRUCTURETYPE_UNIONWITHSUBTYPEDVALUES:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    default: return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Copy NodeIds */
    res = UA_NodeId_copy(&sd->defaultEncodingId, &type->binaryEncodingId);
    UA_CHECK_STATUS(res, UA_DataType_clear(type); return res);

    /* Allocate the members array */
    type->members = (UA_DataTypeMember *)
        UA_calloc(sd->fieldsSize, sizeof(UA_DataTypeMember));
    if(!type->members) {
        UA_DataType_clear(type);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    type->membersSize = (UA_Byte)sd->fieldsSize;

    /* Try to get pointerFree and overlayable handling shortcuts.
     * Adjusted later for each member and end-padding. */
    if(sd->structureType == UA_STRUCTURETYPE_STRUCTURE) {
        type->pointerFree = true;
        type->overlayable = true;
    } else if(sd->structureType == UA_STRUCTURETYPE_UNION) {
        type->pointerFree = true;
    }

    /* Populate the members array */
    for(size_t i = 0; i < sd->fieldsSize; i++) {
        const UA_StructureField *sf = &sd->fields[i];
        UA_DataTypeMember *dtm = &type->members[i];

        /* Find the referenced type */
        dtm->memberType = UA_findDataTypeWithCustom(&sf->dataType, customTypes);
        if(!dtm->memberType) {
            UA_DataType_clear(type);
            return UA_STATUSCODE_BADNOTFOUND;
        }

        /* Update handling shortcuts */
        type->pointerFree &= dtm->memberType->pointerFree;
        type->overlayable &= dtm->memberType->overlayable;

        /* Copy the member name */
#ifdef UA_ENABLE_TYPEDESCRIPTION
        dtm->memberName = (char*)UA_malloc(sf->name.length + 1);
        UA_CHECK(dtm->memberName != NULL,
                 UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
        memcpy((char*)(uintptr_t)dtm->memberName, sf->name.data, sf->name.length);
        *(char*)(uintptr_t)&dtm->memberName[sf->name.length] = '\0';
#endif

        /* Memory size and padding for the scalar case */
        UA_Byte talignment = type_alignment(dtm->memberType);
        size_t memSize = dtm->memberType->memSize;
        dtm->padding = PADDING(type->memSize, talignment);

        /* Handle valuerank and array dimensions */
        if(sf->valueRank == 1) {
            /* 1D-array */
            if(sf->arrayDimensionsSize > 1 ||
               (sf->arrayDimensionsSize == 1 && sf->arrayDimensions[0] != 0)) {
                UA_DataType_clear(type);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            dtm->isArray = true;
            memSize = sizeof(void*) + sizeof(size_t);
            dtm->padding = PADDING(type->memSize, offsetof(struct _pad_size_t, x));
            type->pointerFree = false; /* array is not pointer-free */
        } else if(sf->valueRank != UA_VALUERANK_SCALAR) {
            /* Only 1D-arrays or scalars are allowed */
            UA_DataType_clear(type);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Handle isOptional */
        if(sf->isOptional) {
            if(type->typeKind != UA_DATATYPEKIND_OPTSTRUCT) {
                UA_DataType_clear(type);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            dtm->isOptional = true;
            if(!dtm->isArray) {
                memSize = sizeof(void*);
                dtm->padding = PADDING(type->memSize, offsetof(struct _pad_uintptr_t, x));
            }
            UA_assert(!type->pointerFree); /* Set above */
        }

        /* For unions, leave space for the switchfield in the padding */
        if(type->typeKind == UA_DATATYPEKIND_UNION) {
            UA_Byte fieldPadding = (UA_Byte)((dtm->isArray) ?
                PADDING(sizeof(UA_UInt32), offsetof(struct _pad_size_t, x)) :
                PADDING(sizeof(UA_UInt32), type_alignment(dtm->memberType)));
            dtm->padding = sizeof(UA_UInt32) + fieldPadding;
        }

        /* Adjust the type size for the latest member */
        if(type->typeKind == UA_DATATYPEKIND_UNION) {
            /* Increase the memSize if the current member is the largest */
            if(memSize + dtm->padding > type->memSize)
                type->memSize = (UA_UInt16)(memSize + dtm->padding);
        } else {
            /* Increase the memSize for the current member */
            type->memSize += (UA_UInt16)(memSize + dtm->padding);
        }

        /* Overlayable types cannot have padding */
        if(dtm->padding > 0)
            type->overlayable = false;

        /* TODO: MaxStringLength */
    }

    /* Add final padding according to the member alignment requirements */
    UA_Byte self_alignment = type_alignment(type);
    UA_Byte end_padding = (UA_Byte)(PADDING(type->memSize, self_alignment));
    type->memSize += end_padding;

    /* Finalize handling shortcuts. Types with pointer are never overlayable.  */
    if(end_padding > 0)
        type->overlayable = false;
    type->overlayable &= type->pointerFree;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataType_toStructureDescription(const UA_DataType *type,
                                   UA_StructureDescription *descr) {
    UA_StructureDescription_init(descr);

    UA_StatusCode res = toDescription(type, (UA_DataTypeDescription *)descr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Check if the type can be described by a StructureDefinition.
     * Set baseType and structureType. */
    UA_StructureDefinition *sd = &descr->structureDefinition;
    if(type->typeKind == UA_DATATYPEKIND_STRUCTURE) {
        sd->baseDataType = UA_NS0ID(STRUCTURE);
        sd->structureType = UA_STRUCTURETYPE_STRUCTURE;
    } else if(type->typeKind == UA_DATATYPEKIND_OPTSTRUCT) {
        sd->baseDataType = UA_NS0ID(STRUCTURE);
        sd->structureType = UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS;
    } else if(type->typeKind == UA_DATATYPEKIND_UNION) {
        sd->baseDataType = UA_NS0ID(UNION);
        sd->structureType = UA_STRUCTURETYPE_UNION;
    } else {
        UA_StructureDescription_clear(descr);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Set the DefaultEncodingId */
    res = UA_NodeId_copy(&type->binaryEncodingId, &sd->defaultEncodingId);
    UA_CHECK_STATUS(res, UA_StructureDescription_clear(descr); return res);

    /* Allocate the fields */
    sd->fields = (UA_StructureField*)
        UA_calloc(type->membersSize, sizeof(UA_StructureField));
    if(!sd->fields) {
        UA_StructureDescription_clear(descr);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sd->fieldsSize = type->membersSize;

    /* Populate the fields */
    for(size_t i = 0; i < type->membersSize; i++) {
        UA_StructureField *sf = &sd->fields[i];
        const UA_DataTypeMember *dtm = &type->members[i];
#ifdef UA_ENABLE_TYPEDESCRIPTION
        sf->name = UA_String_fromChars(dtm->memberName);
#endif
        res |= UA_NodeId_copy(&dtm->memberType->typeId, &sf->dataType);
        sf->valueRank = (dtm->isArray) ? UA_VALUERANK_ONE_DIMENSION : UA_VALUERANK_SCALAR;
        sf->isOptional = dtm->isOptional;
        if(dtm->isArray && (type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
                            type->typeKind == UA_DATATYPEKIND_OPTSTRUCT)) {
            sf->arrayDimensions = (UA_UInt32*)UA_malloc(sizeof(UA_UInt32));
            if(!sf->arrayDimensions) {
                UA_StructureDescription_clear(descr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            sf->arrayDimensionsSize = 1;
            *sf->arrayDimensions = 0;
        }
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_StructureDescription_clear(descr);
    return res;
}

/*************/
/* Enum Type */
/*************/

static UA_StatusCode
UA_DataType_fromEnumDescription(UA_DataType *type,
                                const UA_EnumDescription *descr) {
    /* If the builtInType is Int32, the DataType is an Enumeration. If the
     * builtInType is one of the UInteger DataTypes or ExtensionObject, the
     * DataType is an OptionSet. */
    if(descr->builtInType != UA_DATATYPEKIND_INT32 + 1)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Set the basic description */
    UA_StatusCode res = fromDescription(type, (const UA_DataTypeDescription*)descr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the enum description */
    type->typeKind = UA_DATATYPEKIND_ENUM;
    type->memSize = sizeof(UA_Int32);
    type->pointerFree = true;
    type->overlayable = true;

    /* Allocate the members array */
    type->members = (UA_DataTypeMember *)
        UA_calloc(descr->enumDefinition.fieldsSize, sizeof(UA_DataTypeMember));
    if(!type->members) {
        UA_DataType_clear(type);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    type->membersSize = (UA_Byte)descr->enumDefinition.fieldsSize;

    /* Copy the enum fields into the members array */
    for(size_t i = 0; i < type->membersSize; i++) {
        UA_DataTypeMember *dtm = &type->members[i];
        const UA_EnumField *ef = &descr->enumDefinition.fields[i];
        dtm->memberType = (const UA_DataType*)(uintptr_t)ef->value;
#ifdef UA_ENABLE_TYPEDESCRIPTION
        dtm->memberName = (char*)UA_malloc(ef->name.length + 1);
        UA_CHECK(dtm->memberName != NULL,
                 UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
        memcpy((char*)(uintptr_t)dtm->memberName, ef->name.data, ef->name.length);
        *(char*)(uintptr_t)&dtm->memberName[ef->name.length] = '\0';
#endif
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataType_toEnumDescription(const UA_DataType *type,
                              UA_EnumDescription *descr) {
    UA_StatusCode res = toDescription(type, (UA_DataTypeDescription *)descr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the builtin type */
    descr->builtInType = UA_DATATYPEKIND_INT32 + 1;

    /* Allocate the enum fields */
    descr->enumDefinition.fields = (UA_EnumField*)
        UA_calloc(type->membersSize, sizeof(UA_EnumField));
    if(!descr->enumDefinition.fields) {
        UA_EnumDescription_clear(descr);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    descr->enumDefinition.fieldsSize = type->membersSize;

    /* Set the enum fields */
    for(size_t i = 0; i < type->membersSize; i++) {
        const UA_DataTypeMember *dtm = &type->members[i];
        UA_EnumField *ef = &descr->enumDefinition.fields[i];
        ef->value = (UA_Int64)(uintptr_t)dtm->memberType;
#ifdef UA_ENABLE_TYPEDESCRIPTION
        ef->name = UA_STRING_ALLOC(dtm->memberName);
        ef->displayName = UA_LOCALIZEDTEXT_ALLOC("", dtm->memberName);
#endif
    }

    return UA_STATUSCODE_GOOD;
}

/***************/
/* Simple Type */
/***************/

static UA_StatusCode
UA_DataType_fromSimpleTypeDescription(UA_DataType *type,
                                      const UA_SimpleTypeDescription *descr) {
    /* Check if the BuiltinType is a "simple type" */
    if(descr->builtInType > 0 &&
       descr->builtInType > UA_DATATYPEKIND_DIAGNOSTICINFO + 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set the basic description. Then use defaults from the builtin type */
    UA_StatusCode res = fromDescription(type, (const UA_DataTypeDescription*)descr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the type description */
    type->typeKind = UA_TYPES[descr->builtInType-1].typeKind;
    type->memSize = UA_TYPES[descr->builtInType-1].memSize;
    type->pointerFree = UA_TYPES[descr->builtInType-1].pointerFree;
    type->overlayable = UA_TYPES[descr->builtInType-1].overlayable;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataType_toSimpleTypeDescription(const UA_DataType *type,
                                    UA_SimpleTypeDescription *descr) {
    /* Check if the BuiltinType is a "simple type" */
    if(type->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_DataType *baseType = &UA_TYPES[type->typeKind];
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= toDescription(type, (UA_DataTypeDescription *)descr);
    res |= UA_NodeId_copy(&baseType->typeId, &descr->baseDataType);
    descr->builtInType = type->typeKind + 1;

    if(res != UA_STATUSCODE_GOOD)
        UA_SimpleTypeDescription_clear(descr);
    return res;
}

/**************/
/* Public API */
/**************/

UA_StatusCode
UA_DataType_fromDescription(UA_DataType *type, const UA_ExtensionObject *descr,
                            const UA_DataTypeArray *customTypes) {
    void *data = descr->content.decoded.data;
    if(UA_ExtensionObject_hasDecodedType(descr, &UA_TYPES[UA_TYPES_SIMPLETYPEDESCRIPTION]))
        return UA_DataType_fromSimpleTypeDescription(type, (UA_SimpleTypeDescription*)data);
    if(UA_ExtensionObject_hasDecodedType(descr, &UA_TYPES[UA_TYPES_ENUMDESCRIPTION]))
        return UA_DataType_fromEnumDescription(type, (UA_EnumDescription*)data);
    if(UA_ExtensionObject_hasDecodedType(descr, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]))
        return UA_DataType_fromStructureDescription(type, (UA_StructureDescription*)data, customTypes);
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

UA_StatusCode
UA_DataType_toDescription(const UA_DataType *type, UA_ExtensionObject *descr) {
    UA_ExtensionObject_init(descr);
    void *descr_data = NULL;
    const UA_DataType *descr_type = NULL;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(type->typeKind) {
    case UA_DATATYPEKIND_ENUM:
        descr_data = UA_EnumDescription_new();
        if(!descr_data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        descr_type = &UA_TYPES[UA_TYPES_ENUMDESCRIPTION];
        res = UA_DataType_toEnumDescription(type,
                          (UA_EnumDescription*)descr_data);
        break;
    case UA_DATATYPEKIND_STRUCTURE:
    case UA_DATATYPEKIND_OPTSTRUCT:
    case UA_DATATYPEKIND_UNION:
        descr_data = UA_StructureDescription_new();
        if(!descr_data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        descr_type = &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION];
        res = UA_DataType_toStructureDescription(type,
                          (UA_StructureDescription*)descr_data);
        break;
    default:
        descr_data = UA_SimpleTypeDescription_new();
        if(!descr_data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        descr_type = &UA_TYPES[UA_TYPES_SIMPLETYPEDESCRIPTION];
        res = UA_DataType_toSimpleTypeDescription(type,
                          (UA_SimpleTypeDescription*)descr_data);
        break;
    }

    if(res != UA_STATUSCODE_GOOD) {
        UA_free(descr_data);
        return res;
    }

    UA_ExtensionObject_setValue(descr, descr_data, descr_type);
    return UA_STATUSCODE_GOOD;
}
