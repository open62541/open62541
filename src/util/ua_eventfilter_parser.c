/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#include "ua_eventfilter_parser.h"
#include <open62541/types.h>
#include <open62541/types_generated_handling.h>
#include "open62541/util.h"

#include "cj5.h"
#include "mp_printf.h"

static Operand *
OperandList_newOperand(OperandList *ol) {
    Operand *op = (Operand*)UA_calloc(1, sizeof(Operand));
    TAILQ_INSERT_TAIL(ol, op, entries);
    return op;
}

static Operand *
OperandList_find(OperandList *ol, char *ref) {
    Operand *temp;
    TAILQ_FOREACH(temp, ol, entries) {
        if(strcmp(temp->ref, ref) == 0)
            return temp;
    }
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Failed to find the operand with reference: %s", ref);
    return temp;
}

static Operand *
OperandList_resolve(OperandList *ol, Operand *op) {
    if(!op)
        return NULL;
    if(op->type != OT_REF)
        return op;
    return OperandList_resolve(ol, OperandList_find(ol, op->operand.ref));
}

static void
Operand_delete(Operand *on) {
    UA_free(on->ref);
    if(on->type == OT_OPERATOR) {
        UA_free(on->operand.op.children);
    } else if(on->type == OT_REF) {
        UA_free(on->operand.ref);
    } else if(on->type == OT_SAO) {
        UA_SimpleAttributeOperand_clear(&on->operand.sao);
    } else if(on->type == OT_LITERAL) {
        UA_Variant_clear(&on->operand.literal);
    }
    UA_free(on);
}

void
OperandList_clear(OperandList *ol) {
    Operand *temp, *temp1;
    TAILQ_FOREACH_SAFE(temp, ol, entries, temp1) {
        TAILQ_REMOVE(ol, temp, entries);
        Operand_delete(temp);
    }
}

char *
save_string(char *str) {
    char *local_str = (char*) UA_calloc(strlen(str)+1, sizeof(char));
    strcpy(local_str, str);
    return local_str;
}

Operand *
create_operand(OperandList *ol, OperandType ot) {
    Operand *on = OperandList_newOperand(ol);
    on->type = ot;
    return on;
}

Operand *
create_operator(OperandList *ol, UA_FilterOperator fo) {
    Operand *on = create_operand(ol, OT_OPERATOR);
    on->operand.op.filter = fo;
    return on;
}

void
append_operand(Operator *op, Operand *on) {
    op->children = (Operand**)UA_realloc(op->children, (op->childrenSize + 1) * sizeof(Operand*));
    op->children[op->childrenSize] = on;
    op->childrenSize++;
}

UA_StatusCode
append_sao(UA_EventFilter *filter, Operand *sao) {
    return UA_Array_append((void **)&filter->selectClauses,
                           &filter->selectClausesSize, &sao->operand.sao,
                           &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
}

Operand *
parse_literal(OperandList *ol, char *yytext, const UA_DataType *type) {
    Operand *on = OperandList_newOperand(ol);
    on->type = OT_LITERAL;

    void *data = UA_new(type);
    if(!data)
        return on;
    UA_String src = UA_STRING(yytext);
    UA_StatusCode res = UA_decodeJson(&src, data, type, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(data);
        return on;
    }

    UA_Variant_setScalar(&on->operand.literal, data, type);
    return on;
}

/* Count the number of elements for the filter. Mark all required elements that
 * appear in the hierarchy from the top element. */
static size_t
countElements(Operand *top) {
    if(top->type != OT_OPERATOR)
        return 0;
    if(top->operand.op.required)
        return 0; /* already visited */
    top->operand.op.required = true;
    size_t count = 1;
    for(size_t i = 0; i < top->operand.op.childrenSize; i++)
        count += countElements(top->operand.op.children[i]);
    return count;
}

/* Return the first printable operator from the list. It must be required and
 * all child-operators must be printed already. */
static Operator *
getPrintable(OperandList *ol) {
    Operand *on;
    TAILQ_FOREACH(on, ol, entries) {
        if(on->type != OT_OPERATOR)
            continue;

        Operator *op = &on->operand.op;
        if(!op->required || op->elementIndex > 0)
            continue; /* Not required or already printed */

        /* Check all children */
        size_t i = 0;
        for(; i < op->childrenSize; i++) {
            Operand *op_i = OperandList_resolve(ol, op->children[i]);
            if(!op_i)
                return NULL; /* Not allowed */
            if(op_i->type == OT_OPERATOR && op_i->operand.op.elementIndex == 0)
                break;
        }
        if(i == op->childrenSize)
            return op; /* All children are ready */
    }
    return NULL;
}

static UA_StatusCode
print_element(OperandList *ol, UA_ContentFilterElement *elm, Operator *op) {
    UA_ContentFilterElement_init(elm);
    elm->filterOperandsSize = op->childrenSize;
    elm->filterOperands = (UA_ExtensionObject*)
        UA_calloc(op->childrenSize, sizeof(UA_ExtensionObject));
    elm->filterOperator = op->filter;
    for(size_t i = 0; i < op->childrenSize; i++) {
        Operand *op_i = OperandList_resolve(ol, op->children[i]);
        if(op_i->type == OT_OPERATOR) {
            UA_ElementOperand *elmo = UA_ElementOperand_new();
            elmo->index = op_i->operand.op.elementIndex;
            UA_ExtensionObject_setValue(&elm->filterOperands[i], elmo, &UA_TYPES[UA_TYPES_ELEMENTOPERAND]);
        } else if(op_i->type == OT_SAO) {
            UA_SimpleAttributeOperand *sao = UA_SimpleAttributeOperand_new();
            UA_SimpleAttributeOperand_copy(&op_i->operand.sao, sao);
            UA_ExtensionObject_setValue(&elm->filterOperands[i], sao, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        } else if(op_i->type == OT_LITERAL) {
            UA_LiteralOperand *lit = UA_LiteralOperand_new();
            UA_Variant_copy(&op_i->operand.literal, &lit->value);
            UA_ExtensionObject_setValue(&elm->filterOperands[i], lit, &UA_TYPES[UA_TYPES_LITERALOPERAND]);
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
content_filter(OperandList *ol, UA_ContentFilter *filter, Operand *top) {
    size_t count = countElements(top); /* Count relevant filter elements */

    /* Allocate the elements array */
    filter->elements = (UA_ContentFilterElement*)
        UA_Array_new(count, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
    filter->elementsSize = count;

    /* Get the next printable operator and print it */
    Operator *printable;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(count > 0 && (printable = getPrintable(ol))) {
        count--;
        res |= print_element(ol, &filter->elements[count], printable);
        printable->elementIndex = count;
    }

    /* Cycles are not allowed. Detected if we could not print all relevant
     * elements */
    if(count > 0)
        res |= UA_STATUSCODE_BADINTERNALERROR;

    return res;
}
