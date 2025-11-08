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

UA_StatusCode
UA_DataType_fromStructureDefinition(UA_DataType *type,
                                    const UA_StructureDefinition *sd,
                                    const UA_NodeId typeId,
                                    const UA_String typeName,
                                    const UA_DataTypeArray *customTypes) {
    memset(type, 0, sizeof(UA_DataType));

    /* Ensure the type is not already defined */
    const UA_DataType *duplicate = UA_findDataTypeWithCustom(&typeId, customTypes);
    if(duplicate)
        return UA_STATUSCODE_BADALREADYEXISTS;

    /* Set the typeKind */
    switch(sd->structureType) {
    case UA_STRUCTURETYPE_STRUCTURE:
        type->typeKind = UA_DATATYPEKIND_STRUCTURE; break;
    case UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS:
        type->typeKind = UA_DATATYPEKIND_OPTSTRUCT; break;
    case UA_STRUCTURETYPE_UNION:
        type->typeKind = UA_DATATYPEKIND_UNION; break;
    case UA_STRUCTURETYPE_STRUCTUREWITHSUBTYPEDVALUES:
    case UA_STRUCTURETYPE_UNIONWITHSUBTYPEDVALUES:
    default: return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Copy NodeIds */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_NodeId_copy(&typeId , &type->typeId);
    res |= UA_NodeId_copy(&sd->defaultEncodingId, &type->binaryEncodingId);
    UA_CHECK_STATUS(res, UA_DataType_clear(type); return res);

    /* Copy the name */
#ifdef UA_ENABLE_TYPEDESCRIPTION
    type->typeName = (char*)UA_malloc(typeName.length + 1);
    UA_CHECK(type->typeName != 0,
             UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
    memcpy((void*)(uintptr_t)type->typeName, typeName.data, typeName.length);
    *(char*)(uintptr_t)&type->typeName[typeName.length] = '\0';
#endif

    /* Allocate the members array */
    type->members = (UA_DataTypeMember *)
        UA_calloc(sd->fieldsSize, sizeof(UA_DataTypeMember));
    if(!type->members) {
        UA_DataType_clear(type);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    type->membersSize = (UA_UInt32)sd->fieldsSize;

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

        /* Copy the member name */
#ifdef UA_ENABLE_TYPEDESCRIPTION
        dtm->memberName = (char*)UA_malloc(sf->name.length + 1);
        UA_CHECK(dtm->memberName != NULL,
                 UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
        memcpy((char*)(uintptr_t)dtm->memberName, sf->name.data, sf->name.length);
        *(char*)(uintptr_t)&dtm->memberName[sf->name.length] = '\0';
#endif

        /* Memory size and padding for the scalar case */
        UA_Byte alignment = type_alignment(dtm->memberType);
        size_t memSize = dtm->memberType->memSize;
        dtm->padding = PADDING(type->memSize, alignment);

        /* Handle valuerank and array dimensions */
        if(sf->valueRank == 1) {
            if(sf->arrayDimensionsSize > 1 ||
               (sf->arrayDimensionsSize == 1 && sf->arrayDimensions[0] != 0)) {
                UA_DataType_clear(type);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            dtm->isArray = true;
            memSize = sizeof(void*) + sizeof(size_t);
            dtm->padding = PADDING(type->memSize, offsetof(struct _pad_size_t, x));
        } else if(sf->valueRank != UA_VALUERANK_SCALAR) {
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
        }

        /* For unions, leave space for the switchfield in the padding */
        if(type->typeKind == UA_DATATYPEKIND_UNION) {
            UA_Byte fieldPadding = (dtm->isArray) ?
                PADDING(sizeof(UA_UInt32), offsetof(struct _pad_size_t, x)) :
                PADDING(sizeof(UA_UInt32), type_alignment(dtm->memberType));
            dtm->padding = sizeof(UA_UInt32) + fieldPadding;
        }

        /* Adjust the type size for the latest member */
        if(type->typeKind == UA_DATATYPEKIND_UNION) {
            /* Increase the memSize if the current member is the largest */
            if(memSize + dtm->padding > type->memSize)
                type->memSize = (UA_UInt32)(memSize + dtm->padding);
        } else {
            /* Increase the memSize for the current member */
            type->memSize += (UA_UInt32)(memSize + dtm->padding);
        }

        /* TODO: MaxStringLength */
    }

    /* Add final padding according to the member alignment requirements */
    UA_Byte self_alignment = type_alignment(type);
    type->memSize += PADDING(type->memSize, self_alignment);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataType_toStructureDefinition(const UA_DataType *type,
                                  UA_StructureDefinition *sd) {
    UA_StructureDefinition_init(sd);

    /* Check if the type can be described by a StructureDefinition.
     * Set baseType and structureType. */
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
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Set the DefaultEncodingId */
    UA_StatusCode res =
        UA_NodeId_copy(&type->binaryEncodingId, &sd->defaultEncodingId);
    UA_CHECK_STATUS(res, return res);

    /* Allocate the fields */
    sd->fields = (UA_StructureField*)
        UA_calloc(type->membersSize, sizeof(UA_StructureField));
    if(!sd->fields) {
        UA_StructureDefinition_clear(sd);
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
                UA_StructureDefinition_clear(sd);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            sf->arrayDimensionsSize = 1;
            *sf->arrayDimensions = 0;
        }
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_StructureDefinition_clear(sd);
    return res;
}
