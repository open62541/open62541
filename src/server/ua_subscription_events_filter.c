/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

typedef struct {
    UA_Server *server;
    UA_Session *session;
    const UA_NodeId *eventNode;
    const UA_ContentFilter *contentFilter;
    UA_ContentFilterResult *contentFilterResult;
    UA_Variant *valueResult;
    UA_UInt16 index;
} UA_FilterOperatorContext;

static UA_StatusCode
evaluateWhereClauseContentFilter(UA_FilterOperatorContext *ctx);

/* Resolves a variant of type string or boolean into a corresponding status code */
static UA_StatusCode
resolveBoolean(UA_Variant operand) {
    UA_String value;
    value = UA_STRING("True");
    if(((operand.type == &UA_TYPES[UA_TYPES_STRING]) &&
        (UA_String_equal((UA_String *)operand.data, &value))) ||
       ((operand.type == &UA_TYPES[UA_TYPES_BOOLEAN]) &&
        (*(UA_Boolean *)operand.data == UA_TRUE))) {
        return UA_STATUSCODE_GOOD;
    }
    value = UA_STRING("False");
    if(((operand.type == &UA_TYPES[UA_TYPES_STRING]) &&
        (UA_String_equal((UA_String *)operand.data, &value))) ||
       ((operand.type == &UA_TYPES[UA_TYPES_BOOLEAN]) &&
        (*(UA_Boolean *)operand.data == UA_FALSE))) {
        return UA_STATUSCODE_BADNOMATCH;
    }

    /* If the operand can't be resolved, an error is returned */
    return UA_STATUSCODE_BADFILTEROPERANDINVALID;
}

/* Part 4: 7.4.4.5 SimpleAttributeOperand
 * The clause can point to any attribute of nodes. Either a child of the event
 * node and also the event type. */
static UA_StatusCode
resolveSimpleAttributeOperand(UA_Server *server, UA_Session *session,
                              const UA_NodeId *origin,
                              const UA_SimpleAttributeOperand *sao,
                              UA_Variant *value) {
    /* Prepare the ReadValueId */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.indexRange = sao->indexRange;
    rvi.attributeId = sao->attributeId;

    UA_DataValue v;

    if(sao->browsePathSize == 0) {
        /* If this list (browsePath) is empty, the Node is the instance of the
         * TypeDefinition. (Part 4, 7.4.4.5) */
        rvi.nodeId = *origin;

        /* A Condition is an indirection. Look up the target node. */
        /* TODO: check for Branches! One Condition could have multiple Branches */
        UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
        if(UA_NodeId_equal(&sao->typeDefinitionId, &conditionTypeId)) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
            UA_StatusCode res = UA_getConditionId(server, origin, &rvi.nodeId);
            if(res != UA_STATUSCODE_GOOD)
                return res;
#else
            return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
        }

        v = UA_Server_readWithSession(server, session, &rvi,
                                      UA_TIMESTAMPSTORETURN_NEITHER);
    } else {
        /* Resolve the browse path, starting from the event-source (and not the
         * typeDefinitionId). */
        UA_BrowsePathResult bpr =
            browseSimplifiedBrowsePath(server, *origin,
                                       sao->browsePathSize, sao->browsePath);
        if(bpr.targetsSize == 0 && bpr.statusCode == UA_STATUSCODE_GOOD)
            bpr.statusCode = UA_STATUSCODE_BADNOTFOUND;
        if(bpr.statusCode != UA_STATUSCODE_GOOD) {
            UA_StatusCode res = bpr.statusCode;
            UA_BrowsePathResult_clear(&bpr);
            return res;
        }

        /* Use the first match */
        rvi.nodeId = bpr.targets[0].targetId.nodeId;
        v = UA_Server_readWithSession(server, session, &rvi,
                                      UA_TIMESTAMPSTORETURN_NEITHER);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Move the result to the output */
    if(v.status == UA_STATUSCODE_GOOD && v.hasValue)
        *value = v.value;
    else
        UA_Variant_clear(&v.value);
    return v.status;
}

/* Resolve operands to variants according to the operand type.
 * Part 4: 7.17.3 Table 142 specifies the allowed types. */
static UA_Variant
resolveOperand(UA_FilterOperatorContext *ctx, UA_UInt16 nr) {
    UA_StatusCode res;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_ExtensionObject *op = &ctx->contentFilter->elements[ctx->index].filterOperands[nr];
    if(op->content.decoded.type == &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]) {
        /* SimpleAttributeOperand */
        res = resolveSimpleAttributeOperand(ctx->server, ctx->session, ctx->eventNode,
                                (UA_SimpleAttributeOperand *)op->content.decoded.data,
                                            &variant);
    } else if(op->content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
        /* LiteralOperand */
        variant = ((UA_LiteralOperand *)op->content.decoded.data)->value;
        res = UA_STATUSCODE_GOOD;
    } else if(op->content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
        /* ElementOperand */
        UA_UInt16 oldIndex = ctx->index;
        ctx->index = (UA_UInt16)((UA_ElementOperand *)op->content.decoded.data)->index;
        res = evaluateWhereClauseContentFilter(ctx);
        variant = ctx->valueResult[ctx->index];
        ctx->index = oldIndex; /* restore the old index */
    } else {
        res = UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }

    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADNOMATCH) {
        variant.type = NULL;
        ctx->contentFilterResult->elementResults[ctx->index].operandStatusCodes[nr] = res;
    }

    return variant;
}

static UA_StatusCode
ofTypeOperator(UA_FilterOperatorContext *ctx) {
    UA_ContentFilterElement *pElement = &ctx->contentFilter->elements[ctx->index];
    UA_Boolean result = false;
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
    if(pElement->filterOperandsSize != 1)
        return UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
    if(pElement->filterOperands[0].content.decoded.type !=
       &UA_TYPES[UA_TYPES_LITERALOPERAND])
        return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;

    UA_LiteralOperand *literalOperand =
        (UA_LiteralOperand *) pElement->filterOperands[0].content.decoded.data;
    if(!UA_Variant_isScalar(&literalOperand->value))
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    if(literalOperand->value.type != &UA_TYPES[UA_TYPES_NODEID] || literalOperand->value.data == NULL)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    UA_NodeId *literalOperandNodeId = (UA_NodeId *) literalOperand->value.data;
    UA_Variant typeNodeIdVariant;
    UA_Variant_init(&typeNodeIdVariant);
    UA_StatusCode readStatusCode =
        readObjectProperty(ctx->server, *ctx->eventNode,
                           UA_QUALIFIEDNAME(0, "EventType"), &typeNodeIdVariant);
    if(readStatusCode != UA_STATUSCODE_GOOD)
        return readStatusCode;

    if(!UA_Variant_isScalar(&typeNodeIdVariant) ||
       typeNodeIdVariant.type != &UA_TYPES[UA_TYPES_NODEID] ||
       typeNodeIdVariant.data == NULL) {
        UA_LOG_ERROR(&ctx->server->config.logger, UA_LOGCATEGORY_SERVER,
                     "EventType has an invalid type.");
        UA_Variant_clear(&typeNodeIdVariant);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* check if the eventtype-nodeid is equal to the given oftype argument */
    result = UA_NodeId_equal((UA_NodeId*) typeNodeIdVariant.data, literalOperandNodeId);
    /* check if the eventtype-nodeid is a subtype of the given oftype argument */
    if(!result)
        result = isNodeInTree_singleRef(ctx->server,
                                        (UA_NodeId*) typeNodeIdVariant.data,
                                        literalOperandNodeId,
                                        UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_Variant_clear(&typeNodeIdVariant);
    if(!result)
        return UA_STATUSCODE_BADNOMATCH;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
andOperator(UA_FilterOperatorContext *ctx) {
    UA_StatusCode firstBoolean_and = resolveBoolean(resolveOperand(ctx, 0));
    if(firstBoolean_and == UA_STATUSCODE_BADNOMATCH) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_BADNOMATCH;
    }
    /* Evaluation of second operand */
    UA_StatusCode secondBoolean = resolveBoolean(resolveOperand(ctx, 1));
    /* Filteroperator AND */
    if(secondBoolean == UA_STATUSCODE_BADNOMATCH) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_BADNOMATCH;
    }
    if((firstBoolean_and == UA_STATUSCODE_GOOD) &&
       (secondBoolean == UA_STATUSCODE_GOOD)) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADFILTERELEMENTINVALID;
}

static UA_StatusCode
orOperator(UA_FilterOperatorContext *ctx) {
    UA_StatusCode firstBoolean_or = resolveBoolean(resolveOperand(ctx, 0));
    if(firstBoolean_or == UA_STATUSCODE_GOOD) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_GOOD;
    }
    /* Evaluation of second operand */
    UA_StatusCode secondBoolean = resolveBoolean(resolveOperand(ctx, 1));
    if(secondBoolean == UA_STATUSCODE_GOOD) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_GOOD;
    }
    if((firstBoolean_or == UA_STATUSCODE_BADNOMATCH) &&
       (secondBoolean == UA_STATUSCODE_BADNOMATCH)) {
        ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
        return UA_STATUSCODE_BADNOMATCH;
    }
    return UA_STATUSCODE_BADFILTERELEMENTINVALID;
}

static UA_Boolean
isNumericUnsigned(UA_UInt32 dataTypeKind){
    if(dataTypeKind == UA_DATATYPEKIND_UINT64 ||
       dataTypeKind == UA_DATATYPEKIND_UINT32 ||
       dataTypeKind == UA_DATATYPEKIND_UINT16 ||
       dataTypeKind == UA_DATATYPEKIND_BYTE)
        return true;
    return false;
}

static UA_Boolean
isNumericSigned(UA_UInt32 dataTypeKind){
    if(dataTypeKind == UA_DATATYPEKIND_INT64 ||
       dataTypeKind == UA_DATATYPEKIND_INT32 ||
       dataTypeKind == UA_DATATYPEKIND_INT16 ||
       dataTypeKind == UA_DATATYPEKIND_SBYTE)
        return true;
    return false;
}

static UA_Boolean
isFloatingPoint(UA_UInt32 dataTypeKind){
    if(dataTypeKind == UA_DATATYPEKIND_FLOAT ||
       dataTypeKind == UA_DATATYPEKIND_DOUBLE)
        return true;
    return false;
}

static UA_Boolean
isStringType(UA_UInt32 dataTypeKind){
    if(dataTypeKind == UA_DATATYPEKIND_STRING ||
       dataTypeKind == UA_DATATYPEKIND_BYTESTRING)
        return true;
    return false;
}

static UA_StatusCode
implicitNumericVariantTransformation(UA_Variant *variant, void *data){
    if(variant->type == &UA_TYPES[UA_TYPES_UINT64]){
        *(UA_UInt64 *)data = *(UA_UInt64 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_UINT32]){
        *(UA_UInt64 *)data = *(UA_UInt32 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_UINT16]){
        *(UA_UInt64 *)data = *(UA_UInt16 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_BYTE]){
        *(UA_UInt64 *)data = *(UA_Byte *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_INT64]){
        *(UA_Int64 *)data = *(UA_Int64 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_INT32]){
        *(UA_Int64 *)data = *(UA_Int32 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_INT16]){
        *(UA_Int64 *)data = *(UA_Int16 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_SBYTE]){
        *(UA_Int64 *)data = *(UA_SByte *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_DOUBLE]){
        *(UA_Double *)data = *(UA_Double *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_DOUBLE]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_SBYTE]){
        *(UA_Double *)data = *(UA_Float *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_DOUBLE]);
    } else {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
implicitNumericVariantTransformationUnsingedToSigned(UA_Variant *variant, void *data){
    if(variant->type == &UA_TYPES[UA_TYPES_UINT64]){
        if(*(UA_UInt64 *)variant->data > UA_INT64_MAX)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        *(UA_Int64 *)data = *(UA_Int64 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_UINT32]){
        *(UA_Int64 *)data = *(UA_Int32 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_UINT16]){
        *(UA_Int64 *)data = *(UA_Int16 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_BYTE]){
        *(UA_Int64 *)data = *(UA_Byte *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_INT64]);
    } else {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
implicitNumericVariantTransformationSignedToUnSigned(UA_Variant *variant, void *data){
    if(*(UA_Int64 *)variant->data < 0)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    if(variant->type == &UA_TYPES[UA_TYPES_INT64]){
        *(UA_UInt64 *)data = *(UA_UInt64 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_INT32]){
        *(UA_UInt64 *)data = *(UA_UInt32 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_INT16]){
        *(UA_UInt64 *)data = *(UA_UInt16 *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else if(variant->type == &UA_TYPES[UA_TYPES_SBYTE]){
        *(UA_UInt64 *)data = *(UA_Byte *)variant->data;
        UA_Variant_setScalar(variant, data, &UA_TYPES[UA_TYPES_UINT64]);
    } else {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    return UA_STATUSCODE_GOOD;
}

/* 0 -> Same Type, 1 -> Implicit Cast, 2 -> Only explicit Cast, -1 -> cast invalid */
static UA_SByte convertLookup[21][21] = {
    { 0, 1,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 1,-1, 2,-1,-1, 1, 1, 1,-1},
    { 2, 0,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 1,-1, 2,-1,-1, 1, 1, 1,-1},
    {-1,-1, 0,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 0,-1, 2,-1, 2, 2, 2,-1, 2,-1, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1, 2,-1,-1, 1,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 0,-1, 2, 2, 2,-1, 2,-1, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1, 2,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 0, 1, 1,-1, 2,-1, 2,-1,-1, 2, 1, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 0, 1,-1, 2, 2, 2,-1,-1, 2, 2, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 2, 0,-1, 2, 2, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 0,-1, 2,-1,-1, 1, 1, 1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1, 0,-1,-1,-1, 2, 1, 1,-1},
    { 1, 1,-1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,-1, 0, 2, 2, 1, 1, 1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 2, 1, 2,-1,-1, 0, 1, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 1, 1,-1, 2, 2, 2,-1,-1, 2, 0, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 2, 1,-1, 2, 2, 2,-1,-1, 2, 2, 0,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0}
};

/* This array maps the index of the
 * standard DataType-Kind order to the
 * order of the type convertion array */
static UA_Byte dataTypeKindIndex[30] = {
    0,   12,  1,  8, 17,
    9,   18, 10, 19,  6,
    4,   14,  3,  7,  2,
    20,  11,  5, 13, 16,
    15, 255,255,255,255,
    255,255,255,255,255
};

/* The OPC UA Standard defines in Part 4 several data type casting-rules. (see
 * 1.04 part 4 Table 122)
 * Return:
 *      0 -> same type
 *      1 -> types can be casted implicit
 *      2 -> types can only be explicitly casted
 *     -1 -> types can't be casted */
static UA_SByte
checkTypeCastingOption(const UA_DataType *cast_target, const UA_DataType *cast_source) {
    UA_Byte firstOperatorTypeKindIndex = dataTypeKindIndex[cast_target->typeKind];
    UA_Byte secondOperatorTypeKindIndex = dataTypeKindIndex[cast_source->typeKind];
    if(firstOperatorTypeKindIndex == UA_BYTE_MAX ||
       secondOperatorTypeKindIndex == UA_BYTE_MAX)
        return -1;

    return convertLookup[firstOperatorTypeKindIndex][secondOperatorTypeKindIndex];
}

/* Compare operation for equal, gt, le, gte, lee
 * UA_STATUSCODE_GOOD if the comparison was true
 * UA_STATUSCODE_BADNOMATCH if the comparison was false
 * UA_STATUSCODE_BADFILTEROPERATORINVALID for invalid operators
 * UA_STATUSCODE_BADTYPEMISMATCH if one of the operands was not numeric
 * ToDo Array-Casting
 */
static UA_StatusCode
compareOperation(UA_Variant *firstOperand, UA_Variant *secondOperand, UA_FilterOperator op) {
    /* get precedence of the operand types */
    UA_Int16 firstOperand_precedence = UA_DataType_getPrecedence(firstOperand->type);
    UA_Int16 secondOperand_precedence = UA_DataType_getPrecedence(secondOperand->type);
    /* if the types are not equal and one of the precedence-ranks is -1, then there is
       no implicit conversion possible and therefore no compare */
    if(!UA_NodeId_equal(&firstOperand->type->typeId, &secondOperand->type->typeId) &&
       (firstOperand_precedence == -1 || secondOperand_precedence == -1)){
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    /* check if the precedence order of the operators is swapped */
    UA_Variant *firstCompareOperand = firstOperand;
    UA_Variant *secondCompareOperand = secondOperand;
    UA_Boolean swapped = false;
    if (firstOperand_precedence < secondOperand_precedence){
        firstCompareOperand = secondOperand;
        secondCompareOperand = firstOperand;
        swapped = true;
    }
    UA_SByte castRule =
        checkTypeCastingOption(firstCompareOperand->type, secondCompareOperand->type);

    if(!(castRule == 0 || castRule == 1)){
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* The operand Data-Types influence the behavior and steps for the comparison.
     * We need to check the operand types and store a rule which is used to select
     * the right behavior afterwards. */
    enum compareHandlingRuleEnum {
        UA_TYPES_EQUAL_ORDERED,
        UA_TYPES_EQUAL_UNORDERED,
        UA_TYPES_DIFFERENT_NUMERIC_UNSIGNED,
        UA_TYPES_DIFFERENT_NUMERIC_SIGNED,
        UA_TYPES_DIFFERENT_NUMERIC_FLOATING_POINT,
        UA_TYPES_DIFFERENT_TEXT,
        UA_TYPES_DIFFERENT_COMPARE_FORBIDDEN,
        UA_TYPES_DIFFERENT_COMPARE_EXPLIC,
        UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_SIGNED,
        UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_UNSIGNED
    } compareHandlingRuleEnum;

    if(castRule == 0 &&
       (UA_DataType_isNumeric(firstOperand->type) ||
        firstCompareOperand->type->typeKind == (UA_UInt32) UA_DATATYPEKIND_DATETIME ||
        firstCompareOperand->type->typeKind == (UA_UInt32) UA_DATATYPEKIND_STRING ||
        firstCompareOperand->type->typeKind == (UA_UInt32) UA_DATATYPEKIND_BYTESTRING)){
        /* Data-Types with a natural order (allow le, gt, lee, gte) */
        compareHandlingRuleEnum = UA_TYPES_EQUAL_ORDERED;
    } else if(castRule == 0){
        /* Data-Types without a natural order (le, gt, lee, gte are not allowed) */
        compareHandlingRuleEnum = UA_TYPES_EQUAL_UNORDERED;
    } else if(castRule == 1 &&
              isNumericSigned(firstOperand->type->typeKind) &&
              isNumericSigned(secondOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_NUMERIC_SIGNED;
    } else if(castRule == 1 &&
              isNumericUnsigned(firstOperand->type->typeKind) &&
              isNumericUnsigned(secondOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_NUMERIC_UNSIGNED;
    } else if(castRule == 1 &&
              isFloatingPoint(firstOperand->type->typeKind) &&
              isFloatingPoint(secondOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_NUMERIC_FLOATING_POINT;
    } else if(castRule == 1 &&
              isStringType(firstOperand->type->typeKind)&&
              isStringType(secondOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_TEXT;
    } else if(castRule == 1 &&
              isNumericSigned(firstOperand->type->typeKind) &&
              isNumericUnsigned(secondOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_SIGNED;
    } else if(castRule == 1 &&
              isNumericSigned(secondOperand->type->typeKind) &&
              isNumericUnsigned(firstOperand->type->typeKind)){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_UNSIGNED;
    } else if(castRule == -1 || castRule == 2){
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_COMPARE_EXPLIC;
    } else {
        compareHandlingRuleEnum = UA_TYPES_DIFFERENT_COMPARE_FORBIDDEN;
    }

    if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_COMPARE_FORBIDDEN)
        return UA_STATUSCODE_BADFILTEROPERATORINVALID;

    if(swapped){
        firstCompareOperand = secondCompareOperand;
        secondCompareOperand = firstCompareOperand;
    }

    if(op == UA_FILTEROPERATOR_EQUALS){
        UA_Byte variantContent[16];
        memset(&variantContent, 0, sizeof(UA_Byte) * 16);
        if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_SIGNED ||
           compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_UNSIGNED ||
           compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_FLOATING_POINT) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformation(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_SIGNED) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformationUnsingedToSigned(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_UNSIGNED) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformationSignedToUnSigned(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_TEXT) {
            firstCompareOperand->type = &UA_TYPES[UA_TYPES_STRING];
            secondCompareOperand->type = &UA_TYPES[UA_TYPES_STRING];
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_COMPARE_FORBIDDEN ||
                  compareHandlingRuleEnum == UA_TYPES_DIFFERENT_COMPARE_EXPLIC ){
            return UA_STATUSCODE_BADFILTEROPERATORINVALID;
        }
        if(UA_order(firstCompareOperand, secondCompareOperand, &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ) {
            return UA_STATUSCODE_GOOD;
        }
    } else {
        UA_Byte variantContent[16];
        memset(&variantContent, 0, sizeof(UA_Byte) * 16);
        if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_SIGNED ||
           compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_UNSIGNED ||
           compareHandlingRuleEnum == UA_TYPES_DIFFERENT_NUMERIC_FLOATING_POINT) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformation(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_SIGNED) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformationUnsingedToSigned(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_SIGNEDNESS_CAST_TO_UNSIGNED) {
            implicitNumericVariantTransformation(firstCompareOperand, variantContent);
            implicitNumericVariantTransformationSignedToUnSigned(secondCompareOperand, &variantContent[8]);
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_TEXT) {
            firstCompareOperand->type = &UA_TYPES[UA_TYPES_STRING];
            secondCompareOperand->type = &UA_TYPES[UA_TYPES_STRING];
        } else if(compareHandlingRuleEnum == UA_TYPES_EQUAL_UNORDERED) {
            return UA_STATUSCODE_BADFILTEROPERATORINVALID;
        } else if(compareHandlingRuleEnum == UA_TYPES_DIFFERENT_COMPARE_FORBIDDEN ||
                  compareHandlingRuleEnum == UA_TYPES_DIFFERENT_COMPARE_EXPLIC) {
            return UA_STATUSCODE_BADFILTEROPERATORINVALID;
        }
        UA_Order gte_result = UA_order(firstCompareOperand, secondCompareOperand,
                                       &UA_TYPES[UA_TYPES_VARIANT]);
        if(op == UA_FILTEROPERATOR_LESSTHAN) {
            if(gte_result == UA_ORDER_LESS) {
                return UA_STATUSCODE_GOOD;
            }
        } else if(op == UA_FILTEROPERATOR_GREATERTHAN) {
            if(gte_result == UA_ORDER_MORE) {
                return UA_STATUSCODE_GOOD;
            }
        } else if(op == UA_FILTEROPERATOR_LESSTHANOREQUAL) {
            if(gte_result == UA_ORDER_LESS || gte_result == UA_ORDER_EQ) {
                return UA_STATUSCODE_GOOD;
            }
        } else if(op == UA_FILTEROPERATOR_GREATERTHANOREQUAL) {
            if(gte_result == UA_ORDER_MORE || gte_result == UA_ORDER_EQ) {
                return UA_STATUSCODE_GOOD;
            }
        }
    }
    return UA_STATUSCODE_BADNOMATCH;
}

static UA_StatusCode
compareOperator(UA_FilterOperatorContext *ctx) {
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
    UA_Variant firstOperand = resolveOperand(ctx, 0);
    if(UA_Variant_isEmpty(&firstOperand))
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    UA_Variant secondOperand = resolveOperand(ctx, 1);
    if(UA_Variant_isEmpty(&secondOperand)) {
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }
    /* ToDo remove the following restriction: Add support for arrays */
    if(!UA_Variant_isScalar(&firstOperand) || !UA_Variant_isScalar(&secondOperand)){
        return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
    }
    return compareOperation(&firstOperand, &secondOperand,
                            ctx->contentFilter->elements[ctx->index].filterOperator);
}

static UA_StatusCode
bitwiseOperator(UA_FilterOperatorContext *ctx) {
    /* The bitwise operators all have 2 operands which are evaluated equally. */
    UA_Variant firstOperand = resolveOperand(ctx, 0);
    if(UA_Variant_isEmpty(&firstOperand)) {
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }
    UA_Variant secondOperand = resolveOperand(ctx, 1);
    if(UA_Variant_isEmpty(&secondOperand)) {
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }

    UA_Boolean bitwiseAnd =
        ctx->contentFilter->elements[ctx->index].filterOperator == UA_FILTEROPERATOR_BITWISEAND;

    /* check if the operators are integers */
    if(!UA_DataType_isNumeric(firstOperand.type) ||
       !UA_DataType_isNumeric(secondOperand.type) ||
       !UA_Variant_isScalar(&firstOperand) ||
       !UA_Variant_isScalar(&secondOperand) ||
       (firstOperand.type == &UA_TYPES[UA_TYPES_DOUBLE]) ||
       (secondOperand.type == &UA_TYPES[UA_TYPES_DOUBLE]) ||
       (secondOperand.type == &UA_TYPES[UA_TYPES_FLOAT]) ||
       (firstOperand.type == &UA_TYPES[UA_TYPES_FLOAT])) {
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }

    /* check which is the return type (higher precedence == bigger integer)*/
    UA_Int16 precedence = UA_DataType_getPrecedence(firstOperand.type);
    if(precedence > UA_DataType_getPrecedence(secondOperand.type)) {
        precedence = UA_DataType_getPrecedence(secondOperand.type);
    }

    switch(precedence){
        case 3:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_INT64];
            UA_Int64 result_int64;
            if(bitwiseAnd) {
                result_int64 = *((UA_Int64 *)firstOperand.data) & *((UA_Int64 *)secondOperand.data);
            } else {
                result_int64 = *((UA_Int64 *)firstOperand.data) | *((UA_Int64 *)secondOperand.data);
            }
            UA_Int64_copy(&result_int64, (UA_Int64 *) ctx->valueResult[ctx->index].data);
            break;
        case 4:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_UINT64];
            UA_UInt64 result_uint64s;
            if(bitwiseAnd) {
                result_uint64s = *((UA_UInt64 *)firstOperand.data) & *((UA_UInt64 *)secondOperand.data);
            } else {
                result_uint64s = *((UA_UInt64 *)firstOperand.data) | *((UA_UInt64 *)secondOperand.data);
            }
            UA_UInt64_copy(&result_uint64s, (UA_UInt64 *) ctx->valueResult[ctx->index].data);
            break;
        case 5:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_INT32];
            UA_Int32 result_int32;
            if(bitwiseAnd) {
                result_int32 = *((UA_Int32 *)firstOperand.data) & *((UA_Int32 *)secondOperand.data);
            } else {
                result_int32 = *((UA_Int32 *)firstOperand.data) | *((UA_Int32 *)secondOperand.data);
            }
            UA_Int32_copy(&result_int32, (UA_Int32 *) ctx->valueResult[ctx->index].data);
            break;
        case 6:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_UINT32];
            UA_UInt32 result_uint32;
            if(bitwiseAnd) {
                result_uint32 = *((UA_UInt32 *)firstOperand.data) & *((UA_UInt32 *)secondOperand.data);
            } else {
                result_uint32 = *((UA_UInt32 *)firstOperand.data) | *((UA_UInt32 *)secondOperand.data);
            }
            UA_UInt32_copy(&result_uint32, (UA_UInt32 *) ctx->valueResult[ctx->index].data);
            break;
        case 8:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_INT16];
            UA_Int16 result_int16;
            if(bitwiseAnd) {
                result_int16 = *((UA_Int16 *)firstOperand.data) & *((UA_Int16 *)secondOperand.data);
            } else {
                result_int16 = *((UA_Int16 *)firstOperand.data) | *((UA_Int16 *)secondOperand.data);
            }
            UA_Int16_copy(&result_int16, (UA_Int16 *) ctx->valueResult[ctx->index].data);
            break;
        case 9:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_UINT16];
            UA_UInt16 result_uint16;
            if(bitwiseAnd) {
                result_uint16 = *((UA_UInt16 *)firstOperand.data) & *((UA_UInt16 *)secondOperand.data);
            } else {
                result_uint16 = *((UA_UInt16 *)firstOperand.data) | *((UA_UInt16 *)secondOperand.data);
            }
            UA_UInt16_copy(&result_uint16, (UA_UInt16 *) ctx->valueResult[ctx->index].data);
            break;
        case 10:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_SBYTE];
            UA_SByte result_sbyte;
            if(bitwiseAnd) {
                result_sbyte = *((UA_SByte *)firstOperand.data) & *((UA_SByte *)secondOperand.data);
            } else {
                result_sbyte = *((UA_SByte *)firstOperand.data) | *((UA_SByte *)secondOperand.data);
            }
            UA_SByte_copy(&result_sbyte, (UA_SByte *) ctx->valueResult[ctx->index].data);
            break;
        case 11:
            ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BYTE];
            UA_Byte result_byte;
            if(bitwiseAnd) {
                result_byte = *((UA_Byte *)firstOperand.data) & *((UA_Byte *)secondOperand.data);
            } else {
                result_byte = *((UA_Byte *)firstOperand.data) | *((UA_Byte *)secondOperand.data);
            }
            UA_Byte_copy(&result_byte, (UA_Byte *) ctx->valueResult[ctx->index].data);
            break;
        default:
            return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
betweenOperator(UA_FilterOperatorContext *ctx) {
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_Variant firstOperand = resolveOperand(ctx, 0);
    UA_Variant secondOperand = resolveOperand(ctx, 1);
    UA_Variant thirdOperand = resolveOperand(ctx, 2);

    if((UA_Variant_isEmpty(&firstOperand) ||
        UA_Variant_isEmpty(&secondOperand) ||
        UA_Variant_isEmpty(&thirdOperand)) ||
       (!UA_DataType_isNumeric(firstOperand.type) ||
        !UA_DataType_isNumeric(secondOperand.type) ||
        !UA_DataType_isNumeric(thirdOperand.type)) ||
       (!UA_Variant_isScalar(&firstOperand) ||
        !UA_Variant_isScalar(&secondOperand) ||
        !UA_Variant_isScalar(&thirdOperand))) {
        return UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }

    /* Between can be evaluated through greaterThanOrEqual and lessThanOrEqual */
    if(compareOperation(&firstOperand, &secondOperand, UA_FILTEROPERATOR_GREATERTHANOREQUAL) == UA_STATUSCODE_GOOD &&
       compareOperation(&firstOperand, &thirdOperand, UA_FILTEROPERATOR_LESSTHANOREQUAL) == UA_STATUSCODE_GOOD){
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADNOMATCH;
}

static UA_StatusCode
inListOperator(UA_FilterOperatorContext *ctx) {
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
    UA_Variant firstOperand = resolveOperand(ctx, 0);

    if(UA_Variant_isEmpty(&firstOperand) ||
       !UA_Variant_isScalar(&firstOperand)) {
        return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
    }

    /* Evaluating the list of operands */
    for(size_t i = 1; i < ctx->contentFilter->elements[ctx->index].filterOperandsSize; i++) {
        /* Resolving the current operand */
        UA_Variant currentOperator = resolveOperand(ctx, (UA_UInt16)i);

        /* Check if the operand conforms to the operator*/
        if(UA_Variant_isEmpty(&currentOperator) ||
           !UA_Variant_isScalar(&currentOperator)) {
            return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
        }
        if(compareOperation(&firstOperand, &currentOperator, UA_FILTEROPERATOR_EQUALS)) {
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNOMATCH;
}

static UA_StatusCode
isNullOperator(UA_FilterOperatorContext *ctx) {
    /* Checking if operand is NULL. This is done by reducing the operand to a
    * variant and then checking if it is empty. */
    UA_Variant operand = resolveOperand(ctx, 0);
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
    if(!UA_Variant_isEmpty(&operand))
        return UA_STATUSCODE_BADNOMATCH;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
notOperator(UA_FilterOperatorContext *ctx) {
    /* Inverting the boolean value of the operand. */
    UA_StatusCode res = resolveBoolean(resolveOperand(ctx, 0));
    ctx->valueResult[ctx->index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
    /* invert result */
    if(res == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNOMATCH;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
evaluateWhereClauseContentFilter(UA_FilterOperatorContext *ctx) {
    UA_LOCK_ASSERT(&ctx->server->serviceMutex, 1);

    if(ctx->contentFilter->elements == NULL || ctx->contentFilter->elementsSize == 0) {
        /* Nothing to do.*/
        return UA_STATUSCODE_GOOD;
    }

    /* The first element needs to be evaluated, this might be linked to other
     * elements, which are evaluated in these cases. See 7.4.1 in Part 4. */
    UA_ContentFilterElement *pElement = &ctx->contentFilter->elements[ctx->index];
    UA_StatusCode *result = &ctx->contentFilterResult->elementResults[ctx->index].statusCode;
    switch(pElement->filterOperator) {
        case UA_FILTEROPERATOR_INVIEW:
            /* Fallthrough */
        case UA_FILTEROPERATOR_RELATEDTO:
            /* Not allowed for event WhereClause according to 7.17.3 in Part 4 */
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
        case UA_FILTEROPERATOR_EQUALS:
            /* Fallthrough */
        case UA_FILTEROPERATOR_GREATERTHAN:
            /* Fallthrough */
        case UA_FILTEROPERATOR_LESSTHAN:
            /* Fallthrough */
        case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
            /* Fallthrough */
        case UA_FILTEROPERATOR_LESSTHANOREQUAL:
            *result = compareOperator(ctx);
            break;
        case UA_FILTEROPERATOR_LIKE:
            return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
        case UA_FILTEROPERATOR_NOT:
            *result = notOperator(ctx);
            break;
        case UA_FILTEROPERATOR_BETWEEN:
            *result = betweenOperator(ctx);
            break;
        case UA_FILTEROPERATOR_INLIST:
            /* ToDo currently only numeric types are allowed */
            *result = inListOperator(ctx);
            break;
        case UA_FILTEROPERATOR_ISNULL:
            *result = isNullOperator(ctx);
            break;
        case UA_FILTEROPERATOR_AND:
            *result = andOperator(ctx);
            break;
        case UA_FILTEROPERATOR_OR:
            *result = orOperator(ctx);
            break;
        case UA_FILTEROPERATOR_CAST:
            return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
        case UA_FILTEROPERATOR_BITWISEAND:
            *result = bitwiseOperator(ctx);
            break;
        case UA_FILTEROPERATOR_BITWISEOR:
            *result = bitwiseOperator(ctx);
            break;
        case UA_FILTEROPERATOR_OFTYPE:
            *result = ofTypeOperator(ctx);
            break;
        default:
            return UA_STATUSCODE_BADFILTEROPERATORINVALID;
    }

    if(ctx->valueResult[ctx->index].type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        UA_Boolean *res = UA_Boolean_new();
        if(ctx->contentFilterResult->elementResults[ctx->index].statusCode == UA_STATUSCODE_GOOD)
            *res = true;
        else
            *res = false;
        ctx->valueResult[ctx->index].data = res;
    }
    return ctx->contentFilterResult->elementResults[ctx->index].statusCode;
}

/* Exposes the filters For unit tests */
UA_StatusCode
UA_Server_evaluateWhereClauseContentFilter(UA_Server *server, UA_Session *session,
                                           const UA_NodeId *eventNode,
                                           const UA_ContentFilter *contentFilter,
                                           UA_ContentFilterResult *contentFilterResult) {
    if(contentFilter->elementsSize == 0)
        return UA_STATUSCODE_GOOD;
    /* TODO add maximum lenth size to the server config */
    if(contentFilter->elementsSize > 256)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Variant valueResult[256];
    for(size_t i = 0; i < contentFilter->elementsSize; ++i) {
        UA_Variant_init(&valueResult[i]);
    }

    UA_FilterOperatorContext ctx;
    ctx.server = server;
    ctx.session = session;
    ctx.eventNode = eventNode;
    ctx.contentFilter = contentFilter;
    ctx.contentFilterResult = contentFilterResult;
    ctx.valueResult = valueResult;
    ctx.index = 0;

    UA_StatusCode res = evaluateWhereClauseContentFilter(&ctx);
    for(size_t i = 0; i < ctx.contentFilter->elementsSize; i++) {
        if(!UA_Variant_isEmpty(&ctx.valueResult[i]))
            UA_Variant_clear(&ctx.valueResult[i]);
    }
    return res;
}

static UA_Boolean
isValidEvent(UA_Server *server, const UA_NodeId *validEventParent,
             const UA_NodeId *eventId) {
    /* find the eventType variableNode */
    UA_QualifiedName findName = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, *eventId, 1, &findName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    /* Get the EventType Property Node */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    /* Read the Value of EventType Property Node (the Value should be a NodeId) */
    UA_StatusCode retval = readWithReadValue(server, &bpr.targets[0].targetId.nodeId,
                                             UA_ATTRIBUTEID_VALUE, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    const UA_NodeId *tEventType = (UA_NodeId*)tOutVariant.data;

    /* check whether the EventType is a Subtype of CondtionType
     * (Part 9 first implementation) */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(validEventParent, &conditionTypeId) &&
       isNodeInTree_singleRef(server, tEventType, &conditionTypeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_BrowsePathResult_clear(&bpr);
        UA_Variant_clear(&tOutVariant);
        return true;
    }

    /*EventType is not a Subtype of CondtionType
     *(ConditionId Clause won't be present in Events, which are not Conditions)*/
    /* check whether Valid Event other than Conditions */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Boolean isSubtypeOfBaseEvent =
        isNodeInTree_singleRef(server, tEventType, &baseEventTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE);

    UA_BrowsePathResult_clear(&bpr);
    UA_Variant_clear(&tOutVariant);
    return isSubtypeOfBaseEvent;
}

UA_StatusCode
filterEvent(UA_Server *server, UA_Session *session,
            const UA_NodeId *eventNode, UA_EventFilter *filter,
            UA_EventFieldList *efl, UA_EventFilterResult *result) {
    if(filter->selectClausesSize == 0)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    UA_EventFieldList_init(efl);
    efl->eventFields = (UA_Variant *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!efl->eventFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    efl->eventFieldsSize = filter->selectClausesSize;

    /* empty event filter result */
    UA_EventFilterResult_init(result);
    result->selectClauseResultsSize = filter->selectClausesSize;
    result->selectClauseResults = (UA_StatusCode *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->selectClauseResults) {
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    /* prepare content filter result structure */
    if(filter->whereClause.elementsSize != 0) {
        result->whereClauseResult.elementResultsSize = filter->whereClause.elementsSize;
        result->whereClauseResult.elementResults = (UA_ContentFilterElementResult *)
            UA_Array_new(filter->whereClause.elementsSize,
            &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT]);
        if(!result->whereClauseResult.elementResults) {
            UA_EventFieldList_clear(efl);
            UA_EventFilterResult_clear(result);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < result->whereClauseResult.elementResultsSize; ++i) {
            result->whereClauseResult.elementResults[i].operandStatusCodesSize =
            filter->whereClause.elements->filterOperandsSize;
            result->whereClauseResult.elementResults[i].operandStatusCodes =
                (UA_StatusCode *)UA_Array_new(
                    filter->whereClause.elements->filterOperandsSize,
                    &UA_TYPES[UA_TYPES_STATUSCODE]);
            if(!result->whereClauseResult.elementResults[i].operandStatusCodes) {
                UA_EventFieldList_clear(efl);
                UA_EventFilterResult_clear(result);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
        }
    }

    /* Apply the content (where) filter */
    UA_StatusCode res =
        UA_Server_evaluateWhereClauseContentFilter(server, session, eventNode,
                                                   &filter->whereClause, &result->whereClauseResult);
    if(res != UA_STATUSCODE_GOOD){
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return res;
    }

    /* Apply the select filter */
    /* Check if the browsePath is BaseEventType, in which case nothing more
     * needs to be checked */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    for(size_t i = 0; i < filter->selectClausesSize; i++) {
        if(!UA_NodeId_equal(&filter->selectClauses[i].typeDefinitionId, &baseEventTypeId) &&
           !isValidEvent(server, &filter->selectClauses[i].typeDefinitionId, eventNode)) {
            UA_Variant_init(&efl->eventFields[i]);
            /* EventFilterResult currently isn't being used
            notification->result.selectClauseResults[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID; */
            continue;
        }

        /* TODO: Put the result into the selectClausResults */
        resolveSimpleAttributeOperand(server, session, eventNode,
                                      &filter->selectClauses[i], &efl->eventFields[i]);
    }

    return UA_STATUSCODE_GOOD;
}

/*****************************************/
/* Validation of Filters during Creation */
/*****************************************/

/* Initial select clause validation. The following checks are currently performed:
 * - Check if typedefenitionid or browsepath of any clause is NULL
 * - Check if the eventType is a subtype of BaseEventType
 * - Check if attributeId is valid
 * - Check if browsePath contains null
 * - Check if indexRange is defined and if it is parsable
 * - Check if attributeId is value */
void
UA_Event_staticSelectClauseValidation(UA_Server *server,
                                      const UA_EventFilter *eventFilter,
                                      UA_StatusCode *result) {
    /* The selectClause only has to be checked, if the size is not zero */
    if(eventFilter->selectClausesSize == 0)
        return;
    for(size_t i = 0; i < eventFilter->selectClausesSize; ++i) {
        result[i] = UA_STATUSCODE_GOOD;
        /* /typedefenitionid or browsepath of any clause is not NULL ? */
        if(UA_NodeId_isNull(&eventFilter->selectClauses[i].typeDefinitionId)) {
            result[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            continue;
        }
        /*ToDo: Check the following workaround. In UaExpert Event View the selection
        * of the Server Object set up 7 select filter entries by default. The last
        * element ist  from node 2782 (A&C ConditionType). Since the reduced
        * information model dos not contain this type, the result has a brows path of
        * "null" which results in an error. */
        UA_NodeId ac_conditionType = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
        if(UA_NodeId_equal(&eventFilter->selectClauses[i].typeDefinitionId, &ac_conditionType)) {
            continue;
        }
        if(&eventFilter->selectClauses[i].browsePath[0] == NULL) {
            result[i] = UA_STATUSCODE_BADBROWSENAMEINVALID;
            continue;
        }
        /* eventType is a subtype of BaseEventType ? */
        UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        if(!isNodeInTree_singleRef(
            server, &eventFilter->selectClauses[i].typeDefinitionId,
            &baseEventTypeId, UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
            result[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            continue;
        }
        /* attributeId is valid ? */
        if(!((0 < eventFilter->selectClauses[i].attributeId) &&
             (eventFilter->selectClauses[i].attributeId < 28))) {
            result[i] = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            continue;
        }
        /* browsePath contains null ? */
        for(size_t j = 0; j < eventFilter->selectClauses[i].browsePathSize; ++j) {
            if(UA_QualifiedName_isNull(
                &eventFilter->selectClauses[i].browsePath[j])) {
                result[i] = UA_STATUSCODE_BADBROWSENAMEINVALID;
                break;
            }
        }

        /* Get the list of Subtypes from current node */
        UA_ReferenceTypeSet reftypes_interface =
                UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
        UA_ExpandedNodeId *chilTypeNodes = NULL;
        size_t chilTypeNodesSize = 0;
        UA_StatusCode res;
        res = browseRecursive(server, 1, &eventFilter->selectClauses[i].typeDefinitionId,
                        UA_BROWSEDIRECTION_FORWARD, &reftypes_interface, UA_NODECLASS_OBJECTTYPE,
                        true, &chilTypeNodesSize, &chilTypeNodes);
        if(res!=UA_STATUSCODE_GOOD){
            result[i] = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            continue;
        }

        UA_Boolean subTypeContainField = false;
        for (size_t j = 0; j < chilTypeNodesSize; ++j) {
            /* browsPath element is defined in path */
            UA_BrowsePathResult bpr =
                    browseSimplifiedBrowsePath(server, chilTypeNodes[j].nodeId,
                                               eventFilter->selectClauses[i].browsePathSize,
                                               eventFilter->selectClauses[i].browsePath);

            if(bpr.statusCode != UA_STATUSCODE_GOOD){
                UA_BrowsePathResult_clear(&bpr);
                continue;
            }
            subTypeContainField = true;
            UA_BrowsePathResult_clear(&bpr);
        }
        if(!subTypeContainField)
            result[i] = UA_STATUSCODE_BADNODEIDUNKNOWN;

        UA_Array_delete(chilTypeNodes, chilTypeNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

        if(result[i] != UA_STATUSCODE_GOOD)
            continue;
        /*indexRange is defined ? */
        if(!UA_String_equal(&eventFilter->selectClauses[i].indexRange,
                            &UA_STRING_NULL)) {
            /* indexRange is parsable ? */
            UA_NumericRange numericRange = UA_NUMERICRANGE("");
            if(UA_NumericRange_parse(&numericRange,
                                     eventFilter->selectClauses[i].indexRange) !=
               UA_STATUSCODE_GOOD) {
                result[i] = UA_STATUSCODE_BADINDEXRANGEINVALID;
                continue;
            }
            UA_free(numericRange.dimensions);
            /* attributeId is value ? */
            if(eventFilter->selectClauses[i].attributeId != UA_ATTRIBUTEID_VALUE) {
                result[i] = UA_STATUSCODE_BADTYPEMISMATCH;
                continue;
            }
        }
    }
}

/* Initial content filter (where clause) check. Current checks:
 * - Number of operands for each (supported) operator */
UA_StatusCode
UA_Event_staticWhereClauseValidation(UA_Server *server,
                                     const UA_ContentFilter *filter,
                                     UA_ContentFilterResult *result) {
    UA_ContentFilterResult_init(result);
    result->elementResultsSize = filter->elementsSize;
    if(result->elementResultsSize == 0)
        return UA_STATUSCODE_GOOD;
    result->elementResults =
        (UA_ContentFilterElementResult *)UA_Array_new(
            result->elementResultsSize,
            &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT]);
    if(!result->elementResults)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < result->elementResultsSize; ++i) {
        UA_ContentFilterElementResult *er = &result->elementResults[i];
        UA_ContentFilterElement ef = filter->elements[i];
        UA_ContentFilterElementResult_init(er);
        er->operandStatusCodes =
            (UA_StatusCode *)UA_Array_new(
                ef.filterOperandsSize,
                &UA_TYPES[UA_TYPES_STATUSCODE]);
        er->operandStatusCodesSize = ef.filterOperandsSize;

        switch(ef.filterOperator) {
            case UA_FILTEROPERATOR_INVIEW:
            case UA_FILTEROPERATOR_RELATEDTO: {
                /* Not allowed for event WhereClause according to 7.17.3 in Part 4 */
                er->statusCode =
                    UA_STATUSCODE_BADEVENTFILTERINVALID;
                break;
            }
            case UA_FILTEROPERATOR_EQUALS:
            case UA_FILTEROPERATOR_GREATERTHAN:
            case UA_FILTEROPERATOR_LESSTHAN:
            case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
            case UA_FILTEROPERATOR_LESSTHANOREQUAL:
            case UA_FILTEROPERATOR_LIKE:
            case UA_FILTEROPERATOR_CAST:
            case UA_FILTEROPERATOR_BITWISEAND:
            case UA_FILTEROPERATOR_BITWISEOR: {
                if(ef.filterOperandsSize != 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_AND:
            case UA_FILTEROPERATOR_OR: {
                if(ef.filterOperandsSize != 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                for(size_t j = 0; j < 2; ++j) {
                    if(ef.filterOperands[j].content.decoded.type !=
                       &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
                        er->operandStatusCodes[j] =
                            UA_STATUSCODE_BADFILTEROPERANDINVALID;
                        er->statusCode =
                            UA_STATUSCODE_BADFILTEROPERANDINVALID;
                        break;
                    }
                    if(((UA_ElementOperand *)ef.filterOperands[j]
                        .content.decoded.data)->index > filter->elementsSize - 1) {
                        er->operandStatusCodes[j] =
                            UA_STATUSCODE_BADINDEXRANGEINVALID;
                        er->statusCode =
                            UA_STATUSCODE_BADINDEXRANGEINVALID;
                        break;
                    }
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_ISNULL:
            case UA_FILTEROPERATOR_NOT: {
                if(ef.filterOperandsSize != 1) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_INLIST: {
                if(ef.filterOperandsSize <= 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_BETWEEN: {
                if(ef.filterOperandsSize != 3) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_OFTYPE: {
                if(ef.filterOperandsSize != 1) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->operandStatusCodesSize = ef.filterOperandsSize;
                if(ef.filterOperands[0].content.decoded.type !=
                   &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDINVALID;
                    break;
                }
                UA_LiteralOperand *literalOperand =
                    (UA_LiteralOperand *)ef.filterOperands[0]
                        .content.decoded.data;

                /* Make sure the &pOperand->nodeId is a subtype of BaseEventType */
                UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
                if(!isNodeInTree_singleRef(
                    server, (UA_NodeId *)literalOperand->value.data, &baseEventTypeId,
                    UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
                    er->statusCode =
                        UA_STATUSCODE_BADNODEIDINVALID;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            default:
                er->statusCode =
                    UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
                break;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
