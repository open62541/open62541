/* This Source Code Form is subjec&t to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#ifndef UA_EVENTFILTER_PARSER_H_
#define UA_EVENTFILTER_PARSER_H_

#include "open62541/plugin/log_stdout.h"
#include "open62541/types_generated.h"
#include "open62541_queue.h"

_UA_BEGIN_DECLS

typedef enum { OT_ELEMENTREF, OT_EXTENSIONOBJECT } OperandType;

typedef struct {
    OperandType type;
    union {
        char *elementRef;
        UA_ExtensionObject extension;
    } value;
} Operand;

typedef struct {
    UA_FilterOperator filter;
    size_t childrenSize;
    Operand *children;
    size_t ContentFilterArrayPosition;
} Operator;

typedef enum { ET_OPERAND, ET_OPERATOR } ElementType;

typedef struct Element {
    char *ref;
    ElementType type;
    union {
        Operator oper;
        Operand operand;
    } element;
    TAILQ_ENTRY(Element) element_entries;
} Element;

typedef TAILQ_HEAD(ElementList, Element) ElementList;

typedef struct {
    size_t branch_element_number;
    size_t for_operator_reference;
    size_t operand_ctr;
} Counters;

char * save_string(char *str);
void create_next_operand_element(ElementList *elements, Operand *operand, char *ref);
UA_StatusCode create_content_filter(ElementList *elements, UA_ContentFilter *filter, char *first_element, UA_StatusCode status);
void add_new_operator(ElementList *global, char *operator_ref, Operator *element);
UA_StatusCode append_select_clauses(UA_SimpleAttributeOperand **select_clauses, size_t *sao_size, UA_ExtensionObject *extension, UA_StatusCode status);
UA_StatusCode set_up_browsepath(UA_QualifiedName **q_name_list, size_t *size, char *str, UA_StatusCode status);
UA_StatusCode create_literal_operand(char *string, UA_LiteralOperand *lit, UA_StatusCode status);
UA_StatusCode create_nodeId_from_string(char *identifier, UA_NodeId *id, UA_StatusCode status);
void handle_elementoperand(Operand *operand, char *ref);
void handle_sao(UA_SimpleAttributeOperand *simple, Operand *operand);
void add_operand_from_branch(char **ref, size_t *operand_ctr, Operand *operand, ElementList *global);
void add_operand_from_literal(char **ref, size_t *operand_ctr, UA_Variant *lit, ElementList *global);
void add_operand_nodeid(Operator *element, char *nodeid, UA_FilterOperator op);
void set_up_variant_from_nodeId(UA_NodeId *id, UA_Variant *litvalue);
void handle_oftype_nodeId(Operator *element, UA_NodeId *id);
void handle_literal_operand(Operand *operand, UA_LiteralOperand *literalValue);
void handle_between_operator(Operator *element, Operand *operand_1, Operand *operand_2, Operand *operand_3);
void handle_two_operands_operator(Operator *element, Operand *operand_1, Operand *operand_2, UA_FilterOperator *filter);
void init_item_list(ElementList *global, Counters *ctr);
void create_branch_element(ElementList *global, size_t *branch_element_number, UA_FilterOperator filteroperator, char *ref_1, char *ref_2, char **ref);
void handle_for_operator(ElementList *global, size_t *for_operator_reference, char **ref, Operator *element);
void change_element_reference(ElementList *global, char *element_name, char *new_element_reference);
void add_in_list_children(ElementList *global, Operand *oper);
void create_in_list_operator(ElementList *global, Operand *oper, char *element_ref);
void create_nodeid_element(ElementList *elements, UA_NodeId *id, char *ref);
void add_child_operands(Operand *operand_list, size_t operand_list_size, Operator *element, UA_FilterOperator oper);
UA_Variant parse_literal(char *yytext, const UA_DataType *type);

_UA_END_DECLS

#endif /* UA_EVENTFILTER_PARSER_H_ */
