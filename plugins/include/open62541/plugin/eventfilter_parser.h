/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#ifndef OPEN62541_NODEID_SETUP_FUNCTIONS_H
#define OPEN62541_NODEID_SETUP_FUNCTIONS_H



#include "open62541/plugin/log_stdout.h"
#include "open62541/server.h"
#include "open62541/server_config_default.h"
#include <open62541/types_generated.h>
#include <open62541/statuscodes.h>
#include "open62541_queue.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "cj5.h"

_UA_BEGIN_DECLS


typedef struct UA_Parsed_EventFilter {
    UA_EventFilter filter;
    UA_StatusCode status;
}UA_Parsed_EventFilter;


typedef enum OperandIdentifier{
    elementoperand,
    extensionobject
}OperandIdentifier;

typedef struct UA_Parsed_Operand{
    OperandIdentifier identifier;
    union{
        char *element_ref;
        UA_ExtensionObject extension;
    }value;
}UA_Parsed_Operand;

typedef struct UA_Parsed_Operator{
    UA_FilterOperator filter;
    size_t nbr_children;
    UA_Parsed_Operand *children;
    size_t ContentFilterArrayPosition;
}UA_Parsed_Operator;

typedef enum ElementIdentifier{
    parsed_operand,
    parsed_operator
}ElementIdentifier;

typedef union UA_Parsed_Element{
    UA_Parsed_Operator oper;
    UA_Parsed_Operand operand;
}UA_Parsed_Element;

typedef TAILQ_HEAD(parsed_filter_elements, Parsed_Element) parsed_filter_elements;

typedef struct Parsed_Element{
    char *ref;
    ElementIdentifier identifier;
    UA_Parsed_Element element;
    TAILQ_ENTRY(Parsed_Element) element_entries;
}UA_Parsed_Element_List;

typedef struct UA_Element_List{
    struct parsed_filter_elements head;
}UA_Element_List;

typedef union UA_Local_Operand{
    UA_SimpleAttributeOperand sao;
    UA_NodeId id;
    char *str;
    UA_LiteralOperand literal;
} UA_Local_Operand;

typedef struct counters{
    size_t branch_element_number;
    size_t for_operator_reference;
    size_t operand_ctr;
}UA_Counters;

UA_EXPORT void save_string(char *str, char **local_str);
UA_EXPORT void create_next_operand_element(UA_Element_List *elements, UA_Parsed_Operand *operand, char *ref);
UA_EXPORT UA_StatusCode create_content_filter(UA_Element_List *elements, UA_ContentFilter *filter, char *first_element, UA_StatusCode status);
UA_EXPORT void add_new_operator(UA_Element_List *global, char *operator_ref, UA_Parsed_Operator *element);
UA_EXPORT UA_StatusCode append_select_clauses(UA_SimpleAttributeOperand **select_clauses, size_t *sao_size, UA_ExtensionObject *extension, UA_StatusCode status);
UA_EXPORT UA_StatusCode set_up_browsepath(UA_QualifiedName **q_name_list, size_t *size, char *str, UA_StatusCode status);
UA_EXPORT UA_StatusCode create_literal_operand(char *string, UA_LiteralOperand *lit, UA_StatusCode status);
UA_EXPORT UA_StatusCode create_nodeId_from_string(char *identifier, UA_NodeId *id, UA_StatusCode status);
UA_EXPORT void handle_elementoperand(UA_Parsed_Operand *operand, char *ref);
UA_EXPORT void handle_sao(UA_SimpleAttributeOperand *simple, UA_Parsed_Operand *operand);
UA_EXPORT void add_operand_from_branch(char **ref, size_t *operand_ctr, UA_Parsed_Operand *operand, UA_Element_List *global);
UA_EXPORT void set_up_variant_from_nodeId(UA_NodeId *id, UA_Variant *litvalue);
UA_EXPORT void handle_oftype_nodeId(UA_Parsed_Operator *element, UA_NodeId *id);
UA_EXPORT void handle_literal_operand(UA_Parsed_Operand *operand, UA_LiteralOperand *literalValue);
UA_EXPORT void set_up_typeid(UA_Local_Operand *operand);
UA_EXPORT void handle_between_operator(UA_Parsed_Operator *element, UA_Parsed_Operand *operand_1, UA_Parsed_Operand *operand_2, UA_Parsed_Operand *operand_3);
UA_EXPORT void handle_two_operands_operator(UA_Parsed_Operator *element, UA_Parsed_Operand *operand_1, UA_Parsed_Operand *operand_2, UA_FilterOperator *filter);
UA_EXPORT void init_item_list(UA_Element_List *global, UA_Counters *ctr);
UA_EXPORT void create_branch_element(UA_Element_List *global, size_t *branch_element_number, UA_FilterOperator filteroperator, char *ref_1, char *ref_2, char **ref);
UA_EXPORT void handle_for_operator(UA_Element_List *global, size_t *for_operator_reference, char **ref, UA_Parsed_Operator *element);
UA_EXPORT void change_element_reference(UA_Element_List *global, char *element_name, char *new_element_reference);
UA_EXPORT void add_in_list_children(UA_Element_List *global, UA_Parsed_Operand *oper);
UA_EXPORT void create_in_list_operator(UA_Element_List *global, UA_Parsed_Operand *oper, char *element_ref);
UA_EXPORT void append_string(char **string, char *yytext);
UA_EXPORT void set_up_variant_from_bool(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_string(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_bstring(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_float(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_double(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_sbyte(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_statuscode(char *yytext, UA_Variant *litvalue);
UA_EXPORT UA_StatusCode set_up_variant_from_expnodeid(char *yytext, UA_Variant *litvalue, UA_StatusCode status);
UA_EXPORT void set_up_variant_from_time(const char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_byte(char *yytext, UA_Variant *litvalue);
UA_EXPORT UA_StatusCode set_up_variant_from_qname(char *str, UA_Variant *litvalue, UA_StatusCode status);
UA_EXPORT void set_up_variant_from_guid(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_int64(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_localized(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_uint16(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_uint32(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_uint64(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_int16(char *yytext, UA_Variant *litvalue);
UA_EXPORT void set_up_variant_from_int32(char *yytext, UA_Variant *litvalue);
UA_EXPORT void create_nodeid_element(UA_Element_List *elements, UA_NodeId *id, char *ref);
UA_EXPORT void add_child_operands(UA_Parsed_Operand *operand_list, size_t operand_list_size, UA_Parsed_Operator *element, UA_FilterOperator oper);

_UA_END_DECLS
#endif //OPEN62541_NODEID_SETUP_FUNCTIONS_H
