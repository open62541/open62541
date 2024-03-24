/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_eventfilter_parser.h"
#include "open62541/plugin/log_stdout.h"

static Operand *
OperandList_newOperand(OperandList *ol) {
    Operand *op = (Operand*)UA_calloc(1, sizeof(Operand));
    LIST_INSERT_HEAD(&ol->list, op, entries);
    ol->listSize++;
    return op;
}

static Operand *
OperandList_find(OperandList *ol, char *ref) {
    Operand *temp, *found = NULL;
    LIST_FOREACH(temp, &ol->list, entries) {
        if(!temp->ref || strcmp(temp->ref, ref) != 0)
            continue;
        if(found) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Duplicate definition of reference: %s", ref);
            return NULL;
        }
        found = temp;
    }
    if(!found) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to find the operand with reference: %s", ref);
    }
    return found;
}

static Operand *
OperandList_resolve(OperandList *ol, Operand *op, size_t depth) {
    if(depth > ol->listSize)
        return NULL; /* prevent infinite recursion */
    if(!op)
        return NULL;
    if(op->type != OT_REF)
        return op;
    return OperandList_resolve(ol, OperandList_find(ol, op->operand.ref), depth+1);
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
    LIST_FOREACH_SAFE(temp, &ol->list, entries, temp1) {
        LIST_REMOVE(temp, entries);
        Operand_delete(temp);
    }
}

void append_select(OperandList *ol, Operand *on) {
    TAILQ_INSERT_TAIL(&ol->select_list, on, select_entries);
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
    op->children = (Operand**)UA_realloc(op->children,
                                         (op->childrenSize + 1) * sizeof(Operand*));
    op->children[op->childrenSize] = on;
    op->childrenSize++;
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
countElements(OperandList *ol, Operand *top, UA_StatusCode *res) {
    top = OperandList_resolve(ol, top, 0);
    if(!top) {
        *res |= UA_STATUSCODE_BADINTERNALERROR;
        return 0;
    }
    if(top->type != OT_OPERATOR)
        return 0; /* Not an element but the operand of an element */
    if(top->operand.op.required)
        return 0; /* already visited */
    top->operand.op.required = true;
    size_t count = 1;
    for(size_t i = 0; i < top->operand.op.childrenSize; i++)
        count += countElements(ol, top->operand.op.children[i], res);
    return count;
}

/* Return the first printable operator from the list. It must be required and
 * all child-operators must be printed already. */
static Operator *
getPrintable(OperandList *ol) {
    Operand *on;
    LIST_FOREACH(on, &ol->list, entries) {
        if(on->type != OT_OPERATOR)
            continue;

        Operator *op = &on->operand.op;
        if(!op->required || op->elementIndex > 0)
            continue; /* Not required or already printed */

        /* Check all children */
        size_t i = 0;
        for(; i < op->childrenSize; i++) {
            Operand *op_i = OperandList_resolve(ol, op->children[i], 0);
            UA_assert(op_i);
            /* Must resolve. Otherwise countElements would have found the error. */
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
        Operand *op_i = OperandList_resolve(ol, op->children[i], 0);
        if(op_i->type == OT_OPERATOR) {
            UA_ElementOperand *elmo = UA_ElementOperand_new();
            elmo->index = (UA_UInt32)op_i->operand.op.elementIndex;
            UA_ExtensionObject_setValue(&elm->filterOperands[i], elmo,
                                        &UA_TYPES[UA_TYPES_ELEMENTOPERAND]);
        } else if(op_i->type == OT_SAO) {
            UA_SimpleAttributeOperand *sao = UA_SimpleAttributeOperand_new();
            UA_SimpleAttributeOperand_copy(&op_i->operand.sao, sao);
            UA_ExtensionObject_setValue(&elm->filterOperands[i], sao,
                                        &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        } else if(op_i->type == OT_LITERAL) {
            UA_LiteralOperand *lit = UA_LiteralOperand_new();
            UA_Variant_copy(&op_i->operand.literal, &lit->value);
            UA_ExtensionObject_setValue(&elm->filterOperands[i], lit,
                                        &UA_TYPES[UA_TYPES_LITERALOPERAND]);
        }
    }
    return UA_STATUSCODE_GOOD;
}

//#define UA_EVENTFILTERPARSER_DEBUG 1

#ifdef UA_EVENTFILTERPARSER_DEBUG
#include <stdio.h>
static void
debug_element(OperandList *ol, Operand *on) {
    if(on->ref)
        printf("%s: ", on->ref);
    if(on->type == OT_REF) {
        printf("-> %s\n", on->operand.ref);
    } else if(on->type == OT_OPERATOR) {
        printf("Operator %i\n", (int)on->operand.op.filter);
    } else if(on->type == OT_SAO) {
        printf("SAO\n");
    } else if(on->type == OT_LITERAL) {
        printf("Literal\n");
    }
}
#endif

UA_StatusCode
create_filter(OperandList *ol, UA_EventFilter *filter, Operand *top) {
#ifdef UA_EVENTFILTERPARSER_DEBUG
    Operand *temp;
    LIST_FOREACH(temp, &ol->list, entries) {
        debug_element(ol, temp);
    }
#endif

    /* Create the select filter */
    Operand *sao;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    TAILQ_FOREACH(sao, &ol->select_list, select_entries) {
        sao = OperandList_resolve(ol, sao, 0);
        if(!sao)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(sao->type != OT_SAO) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Select Clause not a SimpleAttributeOperand");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        res = UA_Array_append((void **)&filter->selectClauses,
                              &filter->selectClausesSize, &sao->operand.sao,
                              &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Create the where filter */
    if(!top)
        return UA_STATUSCODE_GOOD; /* No where clause */

    size_t count = countElements(ol, top, &res); /* Count relevant filter elements */
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate the elements array */
    filter->whereClause.elements = (UA_ContentFilterElement*)
        UA_Array_new(count, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
    filter->whereClause.elementsSize = count;

    /* Get the next printable operator and print it */
    Operator *printable;
    while(count > 0 && (printable = getPrintable(ol))) {
        count--;
        res |= print_element(ol, &filter->whereClause.elements[count], printable);
        printable->elementIndex = count;
    }

    /* Cycles are not allowed. Detected if we could not print all relevant
     * elements */
    if(count > 0)
        res |= UA_STATUSCODE_BADINTERNALERROR;

    return res;
}
