/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_subscription.h"
#include "ua_query.h"

#ifdef UA_ENABLE_QUERY /* conditional compilation */

UA_Variant *
UA_QueryTable_addRow(UA_QueryTable *qt) {
    if(qt->rowCount >= qt->rowSize) {
        size_t newSize = qt->rowSize * 2;
        UA_Variant **tmp = (UA_Variant**)UA_realloc(qt->rows, sizeof(UA_Variant*) * newSize);
        if(!tmp)
            return NULL;
        qt->rows = tmp;
        qt->rowSize = newSize;
    }
    UA_Variant *row = (UA_Variant*)UA_calloc(qt->columnCount, sizeof(UA_Variant));
    if(UA_LIKELY(row != NULL))
        qt->rows[qt->rowCount++] =  row;
    return row;
}

void
UA_QueryTable_removeRow(UA_QueryTable *qt, size_t index) {
    UA_Array_delete(qt->rows[index], qt->columnCount, &UA_TYPES[UA_TYPES_VARIANT]);
    size_t last = qt->rowCount--;
    qt->rows[index] = qt->rows[last];
}

UA_StatusCode
UA_QueryTable_init(UA_QueryTable *qt, const UA_ContentFilter *filter) {
    memset(qt, 0, sizeof(UA_QueryTable));

    /* Allocate rows */
    qt->rows = (UA_Variant**)UA_malloc(sizeof(UA_Variant*) * 32);
    if(!qt->rows)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    qt->rowSize = 32;

    /* Count columns */
    UA_STACKARRAY(UA_UInt32, opIndex, filter->elementsSize);
    size_t columnCount = 0;
    for(size_t i; i < filter->elementsSize; i++) {
        UA_ContentFilterElement *cfe = &filter->elements[i];
        opIndex[i] = columnCount;
        columnCount++;
        for(size_t j = 0; j < cfe->filterOperandsSize; j++) {
            UA_ExtensionObject *eo = &cfe->filterOperands[j];
            if(eo->encoding != UA_EXTENSIONOBJECT_DECODED &&
               eo->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) {
                UA_QueryTable_clear(qt);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND])
                continue;
            if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND] ||
               eo->content.decoded.type == &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND] ||
               eo->content.decoded.type == &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]) {
                columnCount++;
            } else {
                UA_QueryTable_clear(qt);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
        }
    }

    /* Allocate columns */
    qt->columns = (UA_QueryColumnDescription*)
        UA_calloc(columnCount, sizeof(UA_QueryColumnDescription));
    if(!qt->columns) {
        UA_QueryTable_clear(qt);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    qt->columnCount = columnCount;

    /* Initialize the column descriptions */
    size_t column = columnCount - 1; /* From back to front */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = filter->elementsSize-1; i < filter->elementsSize; i--) {
        UA_UInt32 operands[UA_EVENTFILTER_MAXOPERANDS];
        UA_ContentFilterElement *cfe = &filter->elements[i];
        UA_QueryColumnDescription *qcd = &qt->columns[column--];

        for(size_t j = cfe->filterOperandsSize-1; j < cfe->filterOperandsSize; j--) {
            UA_ExtensionObject *eo = &cfe->filterOperands[j];

            /* Element Operand points to a column that was already set (with a
             * higher index) */
            if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
                UA_ElementOperand *eop = (UA_ElementOperand*)eo->content.decoded.data;
                if(eop->index >= filter->elementsSize || eop->index >= i) {
                    res = UA_STATUSCODE_BADINTERNALERROR;
                    break;
                }
                operands[j] = opIndex[eop->index];
                continue;
            }

            /* Create a dedicated column for the other operands */
            void *data = eo->content.decoded.data;
            if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
                qcd->columnType = UA_QUERYCOLUMNTYPE_LITERAL;
                res |= UA_Variant_copy(&((UA_LiteralOperand*)data)->value, &qcd->content.literal);
            } else if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]) {
                qcd->columnType = UA_QUERYCOLUMNTYPE_ATTRIBUTE;
                res |= UA_AttributeOperand_copy((UA_AttributeOperand*)data, &qcd->content.ao);
            } else /* if(eo->content.decoded.type == &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]) */ {
                qcd->columnType = UA_QUERYCOLUMNTYPE_SIMPLEATTRIBUTE;
                res |= UA_SimpleAttributeOperand_copy((UA_SimpleAttributeOperand*)data,
                                                      &qcd->content.sao);
            }

            operands[j] = column; /* The operands points to this column */
            qcd = &qt->columns[column--];
        }

        qcd->columnType = UA_QUERYCOLUMNTYPE_OPERATOR;
        qcd->content.op.op = cfe->filterOperator;
        qcd->content.op.operandsSize = cfe->filterOperandsSize;
        memcpy(qcd->content.op.operands, operands, sizeof(UA_UInt32) * cfe->filterOperandsSize);
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_QueryTable_clear(qt);
    return res;
}

void
UA_QueryTable_clear(UA_QueryTable *qt) {
    for(size_t i = 0; i < qt->rowCount; i++) {
        UA_Array_delete(qt->rows[i], qt->columnCount, &UA_TYPES[UA_TYPES_VARIANT]);
    }
    UA_free(qt->rows);

    for(size_t i = 0; i < qt->columnCount; i++) {
        UA_QueryColumnDescription *qcd = &qt->columns[i];
        switch(qcd->columnType) {
        case UA_QUERYCOLUMNTYPE_LITERAL:
            UA_Variant_clear(&qcd->content.literal); break;
        case UA_QUERYCOLUMNTYPE_ATTRIBUTE:
            UA_AttributeOperand_clear(&qcd->content.ao); break;
        case UA_QUERYCOLUMNTYPE_SIMPLEATTRIBUTE:
            UA_SimpleAttributeOperand_clear(&qcd->content.sao); break;
        case UA_QUERYCOLUMNTYPE_OPERATOR:
        default:
            break;
        }
    }
    UA_free(qt->columns);

    memset(qt, 0, sizeof(UA_QueryTable));
}

#endif /* UA_ENABLE_QUERY */
