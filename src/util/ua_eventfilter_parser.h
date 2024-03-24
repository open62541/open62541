/* This Source Code Form is subjec&t to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#ifndef UA_EVENTFILTER_PARSER_H_
#define UA_EVENTFILTER_PARSER_H_

#include "open62541/types.h"
#include "open62541_queue.h"

_UA_BEGIN_DECLS

struct Operand;
typedef struct Operand Operand;

typedef struct {
    UA_FilterOperator filter;
    size_t childrenSize;
    Operand **children;
    UA_Boolean required; /* Referenced in the operator hierarchy */
    size_t elementIndex; /* If non-null, then the Operator has been printed out
                          * already to the final EventFilter in the element
                          * array. */
} Operator;

typedef enum { OT_OPERATOR, OT_REF, OT_SAO, OT_LITERAL } OperandType;

struct Operand {
    char *ref; /* The operand has a name */
    OperandType type;
    union {
        Operator op;
        char *ref; /* Reference to another operand with a name */
        UA_SimpleAttributeOperand sao;
        UA_Variant literal;
    } operand;
    LIST_ENTRY(Operand) entries;
    TAILQ_ENTRY(Operand) select_entries;
};

typedef struct {
    LIST_HEAD(, Operand) list;
    TAILQ_HEAD(, Operand) select_list; /* Operands for the select clause in reverse order */
    size_t listSize;
} OperandList;

void OperandList_clear(OperandList *ol);

char * save_string(char *str);
Operand *create_operand(OperandList *ol, OperandType ot);
Operand *create_operator(OperandList *ol, UA_FilterOperator fo);
void append_operand(Operator *op, Operand *on);
void append_select(OperandList *ol, Operand *on);
Operand * parse_literal(OperandList *ol, char *yytext, const UA_DataType *type);
UA_StatusCode create_filter(OperandList *ol, UA_EventFilter *filter, Operand *top);

_UA_END_DECLS

#endif /* UA_EVENTFILTER_PARSER_H_ */
