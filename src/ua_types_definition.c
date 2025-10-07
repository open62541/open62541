/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/util.h>
#include "util/ua_util_internal.h"

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
    UA_CHECK(type->typeName != 0, UA_DataType_clear(type); return UA_STATUSCODE_BADOUTOFMEMORY);
    memcpy((void*)(uintptr_t)type->typeName, typeName.data, typeName.length);
    *(char*)(uintptr_t)&type->typeName[typeName.length] = '\0';
#endif

    /* Allocate the members array */
    type->members = (UA_DataTypeMember*)UA_calloc(sd->fieldsSize, sizeof(UA_DataTypeMember));
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

        /* Handle valuerank and array dimensions */
        size_t memSize = dtm->memberType->memSize;
        if(sf->valueRank == 1) {
            if(sf->arrayDimensionsSize > 1 ||
               (sf->arrayDimensionsSize == 1 &&
                sf->arrayDimensions[0] != 0)) {
                UA_DataType_clear(type);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            dtm->isArray = true;
            memSize = sizeof(void*) + sizeof(size_t);
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
            dtm->isOptional = sf->isOptional;
            if(sf->valueRank == UA_VALUERANK_SCALAR)
                memSize = sizeof(void*);
        }

        /* Define the padding. Since the type does not have a C-struct
         * definition, we are not bound to padding rules. To be compatible with
         * alignment rules, always pad to to round up to the length of
         * size_t. */
        if(i == 0 && type->typeKind == UA_DATATYPEKIND_OPTSTRUCT) {
            /* Initial padding to leave space for the options mask */
            dtm->padding = sizeof(size_t);
        }
        if(i > 0) {
            const UA_DataType *before = type->members[i-1].memberType;
            size_t remainder = before->memSize % sizeof(size_t);
            if(remainder != 0)
                dtm->padding = (UA_Byte)(sizeof(size_t) - remainder);
        }

        /* Increase the length of the overall type */
        type->memSize += (UA_UInt32)(memSize + dtm->padding);

        /* TODO: MaxStringLength */
    }

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
