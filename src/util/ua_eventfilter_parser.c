/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#include "ua_eventfilter_parser.h"
#include <open62541/types.h>
#include "open62541/util.h"

#include "cj5.h"
#include "mp_printf.h"

static Element *
create_next_operator_element(ElementList *elements) {
    Element *element = (Element*)UA_calloc(1, sizeof(Element));
    element->type = ET_OPERATOR;
    element->element.oper.ContentFilterArrayPosition = 0;
    TAILQ_INSERT_TAIL(elements, element, element_entries);
    return element;
}

void
save_string(char *str, char **local_str) {
    *local_str = (char*) UA_calloc(strlen(str)+1, sizeof(char));
    strcpy(*local_str, str);
}

void
create_next_operand_element(ElementList *elements, Operand *operand, char *ref) {
    Element *element = (Element*)UA_calloc(1, sizeof(Element));
    element->type = ET_OPERAND;
    element->ref = ref;
    element->element.operand.type = operand->type ;
    if(operand->type == OT_EXTENSIONOBJECT) {
        element->element.operand.value.extension = operand->value.extension;
        UA_ExtensionObject_init(&operand->value.extension);
    } else {
        save_string(operand->value.element_ref, &element->element.operand.value.element_ref);
    }
    TAILQ_INSERT_TAIL(elements, element, element_entries);
}

static Element *
get_element_by_reference(ElementList *elements, char **reference) {
    Element *temp;
    TAILQ_FOREACH(temp, elements, element_entries) {
        if(strcmp(temp->ref, *reference) == 0)
            return temp;
    }
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Failed to find the element with reference: %s", *reference);
    return temp;
}

static Element *
get_element_by_idx_position(ElementList *elements, size_t idx) {
    Element *temp;
    TAILQ_FOREACH(temp, elements, element_entries) {
        if(temp->element.oper.ContentFilterArrayPosition == idx)
            return temp;
    }
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Failed to find the element with idx %lu.", (unsigned long) idx);
    return temp;
}

static Element *
get_last_element_from_list(ElementList *elements) {
    return TAILQ_LAST(elements, ElementList);
}

static UA_StatusCode
solve_children_references(Element *temp, ElementList *elements,
                          size_t *position, size_t *nbr_children, char ***child_references) {
    for(size_t i = 0; i < temp->element.oper.nbr_children; i++) {
        if(temp->element.oper.children[i].type == OT_ELEMENT_REF) {
            Element *temp_1 =
                get_element_by_reference(elements, &temp->element.oper.children[i].value.element_ref);
            if(temp_1->type == ET_OPERATOR) {
                if(temp_1->element.oper.ContentFilterArrayPosition == 0) {
                    (*nbr_children)++;
                    *child_references = (char**)
                        UA_realloc(*child_references, *nbr_children*sizeof(char*));
                    save_string(temp->element.oper.children[i].value.element_ref,
                                &(*child_references)[*nbr_children-1]);
                    (*position)++;
                    temp_1->element.oper.ContentFilterArrayPosition = *position;
                }
                free(temp->element.oper.children[i].value.element_ref);
                UA_ExtensionObject extension;
                extension.encoding = UA_EXTENSIONOBJECT_DECODED;
                extension.content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
                extension.content.decoded.data = &temp_1->element.oper.ContentFilterArrayPosition;
                UA_ExtensionObject_copy(&extension, &temp->element.oper.children[i].value.extension);
                temp->element.oper.children[i].type = OT_EXTENSIONOBJECT;
            } else {
                if(temp_1->element.operand.type == OT_ELEMENT_REF) {
                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                 "Error! Unsolved reference to an ElementOperand");
                    return UA_STATUSCODE_BAD;
                }
                free(temp->element.oper.children[i].value.element_ref);
                temp->element.oper.children[i].type = OT_EXTENSIONOBJECT;
                UA_ExtensionObject_copy(&temp_1->element.operand.value.extension,
                                        &temp->element.oper.children[i].value.extension);

            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
check_recursion_on_operands(Element *element, size_t init_element,
                            ElementList *elements, size_t *ctr, UA_StatusCode *status){
    if(*status != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOOD;
    for(size_t i = 0; i<element->element.oper.nbr_children; i++) {
        if(element->element.oper.children[i].value.extension.content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]){
            UA_ElementOperand *temp = (UA_ElementOperand*)
                element->element.oper.children[i].value.extension.content.decoded.data;
            Element *next_element =
                get_element_by_idx_position(elements, (size_t) temp->index);
            if(next_element->element.oper.ContentFilterArrayPosition == init_element) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Operator %s has a loop to itself", next_element->ref);
                return UA_STATUSCODE_BAD;
            }
            //when checking an element and a loop exists inside the tree
            //structure, a termination condition is required, otherwise, the
            //loop is detecter, however as it does not involve the current
            //element, the condition based on the element index will never be
            //met
            if(*ctr > 1000) {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Checking Operator on position %lu." PRIu64
                               " Loop within the ContentFilter structure detected",
                               (unsigned long)init_element);
                break;
            }
            (*ctr)++;
            UA_StatusCode retval =
                check_recursion_on_operands(next_element, init_element, elements, ctr, status);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
handle_filter_structure(char *first_element, ElementList *elements, size_t *position) {
    Element *temp;
    size_t nbr_children = 0;
    char **children_list = (char**) UA_calloc(0, sizeof(char*));
    temp = get_element_by_reference(elements, &first_element);
    solve_children_references(temp, elements, position, &nbr_children, &children_list);
    while(nbr_children > 0) {
        size_t temp_ctr = nbr_children;
        char **temp_list = (char**) UA_calloc(temp_ctr, sizeof(char*));
        for(size_t i=0; i< temp_ctr; i++){
            temp_list[i] = (char*) UA_calloc(strlen(children_list[i])+1, sizeof(char));
            strcpy(temp_list[i], children_list[i]);
            free(children_list[i]);
        }
        nbr_children = 0;
        children_list = (char**) UA_realloc(children_list, nbr_children);
        for(size_t i=0; i< temp_ctr; i++){
            Element *child = get_element_by_reference(elements, &temp_list[i]);
            solve_children_references(child, elements, position, &nbr_children, &children_list);
            free(temp_list[i]);
        }
        free(temp_list);
    }
    free(children_list);
    free(first_element);
}

static UA_StatusCode
solve_operand_references(ElementList *elements) {
    size_t current_nbr = 1, last_nbr = 0;
    Element *temp, *temp1;
    while(current_nbr > 0) {
        current_nbr = 0;
        TAILQ_FOREACH(temp, elements, element_entries) {
            if(temp->type == ET_OPERAND){
                if(temp->element.operand.type == OT_ELEMENT_REF) {
                    temp1 = get_element_by_reference(elements,
                                                     &temp->element.operand.value.element_ref);
                    if(temp1->type == ET_OPERATOR) {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                     "Error! An operand references an Operator");
                        return UA_STATUSCODE_BAD;
                    }

                    if(temp1->element.operand.type == OT_ELEMENT_REF) {
                        current_nbr++;
                    } else {
                        free(temp->element.operand.value.element_ref);
                        temp->element.operand.type = OT_EXTENSIONOBJECT;
                        UA_ExtensionObject_copy(&temp1->element.operand.value.extension,
                                                &temp->element.operand.value.extension);
                    }
                }
            }
        }
        if(current_nbr == last_nbr && last_nbr != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Error! Unable to solve all operand references");
            return UA_STATUSCODE_BAD;
        }
        last_nbr = current_nbr;
    }
    return UA_STATUSCODE_GOOD;
}

static void
clear_operand_list(Operand *operand_list, size_t list_size) {
    for(size_t i=0; i< list_size; i++) {
        operand_list[i].type == OT_ELEMENT_REF ?
            free(operand_list[i].value.element_ref) :
            UA_ExtensionObject_clear(&operand_list[i].value.extension);
    }
}

static void
clear_linked_list(ElementList *elements) {
    Element *temp, *temp1;
    TAILQ_FOREACH_SAFE(temp, elements, element_entries, temp1) {
        free(temp->ref);
        if(temp->type == ET_OPERAND) {
            clear_operand_list(&temp->element.operand, 1);
        } else {
            UA_FilterOperator_clear(&temp->element.oper.filter);
            clear_operand_list(temp->element.oper.children, temp->element.oper.nbr_children);
            free(temp->element.oper.children);
        }
        TAILQ_REMOVE(elements, temp, element_entries);
        free(temp);
    }
}

static void
add_content_filter_element(UA_ContentFilterElement *filterElement,
                           Operator *parsedElement) {
    UA_ContentFilterElement_init(filterElement);
    filterElement->filterOperandsSize = parsedElement->nbr_children;
    filterElement->filterOperands = (UA_ExtensionObject*)
        UA_calloc(parsedElement->nbr_children, sizeof(UA_ExtensionObject));
    filterElement->filterOperator = parsedElement->filter;
    for(size_t i=0; i< parsedElement->nbr_children; i++) {
        UA_ExtensionObject_copy(&parsedElement->children[i].value.extension,
                                &filterElement->filterOperands[i]);
        UA_ExtensionObject_clear(&parsedElement->children[i].value.extension);
    }
}

UA_StatusCode
create_content_filter(ElementList *elements, UA_ContentFilter *filter,
                      char *first_element, UA_StatusCode status) {
    if(status != UA_STATUSCODE_GOOD) {
        clear_linked_list(elements);
        free(first_element);
        return status;
    }
    size_t position = 0;
    UA_StatusCode retval = solve_operand_references(elements);
    if(retval!= UA_STATUSCODE_GOOD) {
        clear_linked_list(elements);
        free(first_element);
        return retval;
    }
    handle_filter_structure(first_element, elements, &position);
    ////check for loops inside the tree structure
    Element *temp;
    TAILQ_FOREACH(temp, elements, element_entries) {
        size_t ctr = 0;
        retval = check_recursion_on_operands(temp, temp->element.oper.ContentFilterArrayPosition,
                                             elements, &ctr, &status);
        if(retval != UA_STATUSCODE_GOOD) {
            clear_linked_list(elements);
            return retval;
        }
    }
    //add the elements to the contentFilter based on their array position values
    filter->elementsSize = position+1;
    filter->elements = (UA_ContentFilterElement*)
        UA_Array_new(filter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
    for(size_t i=0; i<=position; i++) {
        temp = get_element_by_idx_position(elements, i);
        add_content_filter_element(&filter->elements[i], &temp->element.oper);
    }
    clear_linked_list(elements);
    return UA_STATUSCODE_GOOD;
}

static void
copy_children(Operator element, Element *temp) {
    for(size_t i=0; i<element.nbr_children; i++) {
        if(element.children[i].type == OT_ELEMENT_REF) {
            temp->element.oper.children[i].type = OT_ELEMENT_REF;
            save_string(element.children[i].value.element_ref,
                        &temp->element.oper.children[i].value.element_ref);
        } else {
            temp->element.oper.children[i].type = OT_EXTENSIONOBJECT;
            UA_ExtensionObject_copy(&element.children[i].value.extension,
                                    &temp->element.oper.children[i].value.extension);
        }
    }
}

static void
add_operator_children(ElementList *global, Operator element) {
    Element *temp = get_last_element_from_list(global);
    memcpy(&temp->element.oper.nbr_children, &element.nbr_children, sizeof(size_t));
    temp->element.oper.children = (Operand*)
        UA_calloc(element.nbr_children, sizeof(Operand));
    UA_FilterOperator_copy(&element.filter, &temp->element.oper.filter);
    copy_children(element, temp);
}

void
add_new_operator(ElementList *global, char *operator_ref,
                 Operator *element) {
    Element *temp = create_next_operator_element(global);
    save_string(operator_ref, &temp->ref);
    free(operator_ref);
    add_operator_children(global, *element);
    clear_operand_list(element->children, element->nbr_children);
    free(element->children);
}

static void
check_SAO(UA_SimpleAttributeOperand *sao) {
    if(sao->typeDefinitionId.identifier.numeric == 0 &&
       sao->typeDefinitionId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        sao->typeDefinitionId = UA_NODEID_NUMERIC(0, 2041);}
    if(sao->attributeId == 0)
        sao->attributeId = 13;
    if(sao->attributeId == 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Extracting a NodeId from a SimpleAttributeOperand "
                     "is not supported in the current implementation");
    }
    //if(sao->indexRange.length == 0) {sao->indexRange =UA_String_fromChars("");}
}

UA_StatusCode
append_select_clauses(UA_SimpleAttributeOperand **select_clauses, size_t *sao_size,
                      UA_ExtensionObject *extension, UA_StatusCode status) {
    if(status != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(extension);
        return status;
    }
    if(*sao_size == 0) {
        *sao_size = 1;
        *select_clauses = (UA_SimpleAttributeOperand*)
            UA_Array_new(*sao_size, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        UA_SimpleAttributeOperand_init(*select_clauses);
        UA_SimpleAttributeOperand_copy((UA_SimpleAttributeOperand*) extension->content.decoded.data,
                                       select_clauses[0]);
    } else {
        UA_StatusCode ret_val = UA_Array_append((void **)select_clauses, sao_size,
                                                (UA_SimpleAttributeOperand*) extension->content.decoded.data,
                                                &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        if(ret_val != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Failed to increase the select clauses %s",
                         UA_StatusCode_name(ret_val));
            UA_ExtensionObject_clear(extension);
            return ret_val;
        }
    }
    UA_ExtensionObject_clear(extension);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
set_up_browsepath(UA_QualifiedName **q_name_list, size_t *size, char *str,
                  UA_StatusCode status) {
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_RelativePath *path = UA_RelativePath_new();
    UA_String parsed_string = UA_String_fromChars(str);
    UA_StatusCode ret_val = UA_RelativePath_parse(path, parsed_string);
    if(ret_val != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to parse the RelativePath %s", UA_StatusCode_name(ret_val));
        UA_RelativePath_delete(path);
        UA_String_clear(&parsed_string);
        return ret_val;
    }
    memcpy(size, &path->elementsSize, sizeof(size_t));
    *q_name_list = (UA_QualifiedName*) UA_calloc(*size, sizeof(UA_QualifiedName));
    for(size_t i=0; i< path->elementsSize; i++)
        UA_QualifiedName_copy(&path->elements[i].targetName, &(*q_name_list)[i]);
    UA_RelativePath_delete(path);
    UA_String_clear(&parsed_string);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
create_literal_operand(char *string, UA_LiteralOperand *lit,
                       UA_StatusCode status) {
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_ByteString input_val = UA_String_fromChars(string);
    UA_StatusCode ret_val = UA_decodeJson(&input_val, (void *)&lit->value,
                                          &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(ret_val != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to decode the Json %s", UA_StatusCode_name(ret_val));
        UA_ByteString_clear(&input_val);
        return ret_val;
    }
    UA_ByteString_clear(&input_val);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
create_nodeId_from_string(char *identifier, UA_NodeId *id, UA_StatusCode status) {
    if(status != UA_STATUSCODE_GOOD) {
        free(identifier);
        return status;
    }
    UA_String str = UA_String_fromChars(identifier);
    free(identifier);
    UA_NodeId_init(id);
    UA_StatusCode ret_val = UA_NodeId_parse(id, str);
    if(ret_val != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to parse the NodeId %s", UA_StatusCode_name(ret_val));
        UA_String_clear(&str);
        return ret_val;
    }
    UA_String_clear(&str);
    return UA_STATUSCODE_GOOD;
}

void
handle_elementoperand(Operand *operand, char *ref) {
    memset(operand, 0, sizeof(Operand));
    operand->type = OT_ELEMENT_REF;
    save_string(ref, &operand->value.element_ref);
}

void handle_sao(UA_SimpleAttributeOperand *simple, Operand *operand) {
    UA_SimpleAttributeOperand *sao = UA_SimpleAttributeOperand_new();
    UA_SimpleAttributeOperand_copy(simple, sao);
    for(size_t i=0; i< simple->browsePathSize; i++) {
        UA_QualifiedName_clear(&simple->browsePath[i]);
    }
    UA_NodeId_clear(&simple->typeDefinitionId);
    UA_String_clear(&simple->indexRange);
    UA_SimpleAttributeOperand_clear(simple);
    check_SAO(sao);
    UA_ExtensionObject_init(&operand->value.extension);
    operand->value.extension.encoding = UA_EXTENSIONOBJECT_DECODED;
    operand->value.extension.content.decoded.type = &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND];
    operand->value.extension.content.decoded.data = sao;
    operand->type = OT_EXTENSIONOBJECT;
}

static void
create_element_reference(size_t *branch_nbr, char **ref, char *ref_identifier) {
    char ref_nbr[128];
    int ret = mp_snprintf(ref_nbr, 128, "%lu" PRIu64 , (unsigned long) *branch_nbr);
    if(ret == 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sprintf Failed");
    }
    *ref = (char*) UA_calloc(strlen(ref_identifier)+1+strlen(ref_nbr), sizeof(char));
    strcpy(*ref, ref_identifier);
    strcat(*ref, ref_nbr);
    (*branch_nbr)++;
}

static void
handle_branch_operator(Element *element, UA_FilterOperator oper,
                       size_t *branch_ctr) {
    element->element.oper.filter = oper;
    create_element_reference(branch_ctr, &element->ref, "operator_reference_");
}

void
add_operand_from_branch(char **ref, size_t *operand_ctr, Operand *operand,
                        ElementList *global) {
    char *temp, temp1[100];
    create_element_reference(operand_ctr, &temp, "operand_reference_");
    strcpy(temp1, temp);
    create_next_operand_element(global, operand, temp);
    save_string(temp1, ref);
}

void
add_operand_from_literal(char **ref, size_t *operand_ctr, UA_Variant *lit, ElementList *global) {
    Element *element = (Element*)UA_calloc(1, sizeof(Element));
    element->type = ET_OPERAND;
    element->ref = *ref;
    element->element.operand.type = OT_EXTENSIONOBJECT;
    UA_ExtensionObject_setValue(&element->element.operand.value.extension, lit->data, lit->type);
    TAILQ_INSERT_TAIL(global, element, element_entries);
}

void
add_operand_nodeid(Operator *element, char *nodeid, UA_FilterOperator filter) {
    element->children = (Operand*)UA_realloc(element->children, (element->nbr_children + 1) * sizeof(Operand));
    element->filter = filter;
    Operand *op = &element->children[element->nbr_children];
    UA_NodeId *id = UA_NodeId_new();
    UA_NodeId_parse(id, UA_STRING(nodeid));
    op->type = OT_EXTENSIONOBJECT;
    UA_ExtensionObject_setValue(&op->value.extension, id, &UA_TYPES[UA_TYPES_NODEID]);
    element->nbr_children++;
}

static void
add_child_references(Element *element, char **ref_1,
                     char **ref_2) {
    element->element.oper.nbr_children = 2;
    element->element.oper.children = (Operand *)UA_calloc(2, sizeof(Operand));
    for(size_t i=0; i<2; i++) {
        element->element.oper.children[i].type = OT_ELEMENT_REF;
        i == 0 ? save_string(*ref_1, &element->element.oper.children[0].value.element_ref)
               : save_string(*ref_2, &element->element.oper.children[1].value.element_ref);
    }
}

static UA_NodeId
copy_nodeId(UA_NodeId *id) {
    UA_NodeId temp;
    UA_NodeId_copy(id, &temp);
    UA_NodeId_clear(id);
    return temp;
}

static void
set_up_ext_object_from_literal(UA_ExtensionObject *obj,
                               UA_LiteralOperand *value) {
    UA_LiteralOperand *lit = UA_LiteralOperand_new();
    UA_LiteralOperand_copy(value, lit);
    UA_LiteralOperand_clear(value);
    UA_ExtensionObject_init(obj);
    obj->encoding = UA_EXTENSIONOBJECT_DECODED;
    obj->content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    obj->content.decoded.data = lit;
}

void
set_up_variant_from_nodeId(UA_NodeId *id, UA_Variant *litvalue) {
    UA_NodeId temp = *id;
    UA_Variant_setScalarCopy(litvalue, &temp, &UA_TYPES[UA_TYPES_NODEID]);
}

void
create_nodeid_element(ElementList *elements, UA_NodeId *id, char *ref) {
    Operand operand;
    memset(&operand, 0, sizeof(Operand));
    UA_LiteralOperand lit;
    set_up_variant_from_nodeId(id, &lit.value);
    UA_NodeId_clear(id);
    set_up_ext_object_from_literal(&operand.value.extension, &lit);
    operand.type = OT_EXTENSIONOBJECT;
    create_next_operand_element(elements, &operand, ref);
}

void
handle_oftype_nodeId(Operator *element, UA_NodeId *id) {
    UA_FilterOperator_init(&element->filter);
    element->filter = UA_FILTEROPERATOR_OFTYPE;
    element->nbr_children = 1;
    element->children = (Operand*) UA_calloc(1, sizeof(Operand));
    UA_LiteralOperand lit;
    set_up_variant_from_nodeId(id, &lit.value);
    UA_NodeId_clear(id);
    set_up_ext_object_from_literal(&element->children[0].value.extension, &lit);
    element->children[0].type = OT_EXTENSIONOBJECT;
}

void
handle_literal_operand(Operand *operand, UA_LiteralOperand *literalValue) {
    set_up_ext_object_from_literal(&operand->value.extension, literalValue);
    operand->type = OT_EXTENSIONOBJECT;
    UA_LiteralOperand_clear(literalValue);
}

void
set_up_typeid(UA_Local_Operand *operand) {
    UA_NodeId temp = copy_nodeId(&operand->id);
    operand->sao.typeDefinitionId = copy_nodeId(&temp);
}

void
add_child_operands(Operand *operand_list, size_t operand_list_size,
                   Operator *element, UA_FilterOperator oper) {
    element->nbr_children = operand_list_size;
    element->filter = oper;
    element->children = (Operand*)UA_calloc(operand_list_size, sizeof(Operand));
    for(size_t i=0; i< operand_list_size; i++) {
        if(operand_list[i].type == OT_ELEMENT_REF) {
            save_string(operand_list[i].value.element_ref, &element->children[i].value.element_ref);
            element->children[i].type = OT_ELEMENT_REF;
            free(operand_list[i].value.element_ref);
        } else {
            UA_ExtensionObject_copy(&operand_list[i].value.extension,
                                    &element->children[i].value.extension);
            element->children[i].type = OT_EXTENSIONOBJECT;
            UA_ExtensionObject_clear(&operand_list[i].value.extension);
        }
    }
}

void
handle_between_operator(Operator *element, Operand *operand_1,
                        Operand *operand_2, Operand *operand_3) {
    Operand operand_list[3] = {*operand_1, *operand_2, *operand_3};
    add_child_operands(operand_list, 3, element, UA_FILTEROPERATOR_BETWEEN);
}

void
handle_two_operands_operator(Operator *element, Operand *operand_1,
                             Operand *operand_2, UA_FilterOperator *filter) {
    Operand operand_list[2] = {*operand_1, *operand_2};
    add_child_operands(operand_list, 2, element, *filter);
}

void
init_item_list(ElementList *global, Counters *ctr) {
    TAILQ_INIT(global);
    ctr->branch_element_number = 0;
    ctr->for_operator_reference = 0;
    ctr->operand_ctr = 0;
}

void
create_branch_element(ElementList *global, size_t *branch_element_number,
                      UA_FilterOperator filteroperator, char *ref_1, char *ref_2, char **ref) {
    Element *temp = create_next_operator_element(global);
    handle_branch_operator(temp, filteroperator, branch_element_number);
    add_child_references(temp, &ref_1, &ref_2);
    save_string(temp->ref, ref);
    free(ref_1);
    free(ref_2);
}

void
handle_for_operator(ElementList *global, size_t *for_operator_reference,
                    char **ref, Operator *element) {
    Element *temp = create_next_operator_element(global);
    create_element_reference(for_operator_reference, &temp->ref, "for_reference_");
    memcpy(&temp->element.oper.nbr_children, &element->nbr_children, sizeof(size_t));
    temp->element.oper.filter = element->filter;
    temp->element.oper.children = (Operand*)UA_calloc(element->nbr_children, sizeof(Operand));
    copy_children(*element, temp);
    save_string(temp->ref, ref);
    clear_operand_list(element->children, element->nbr_children);
    free(element->children);
}

void
change_element_reference(ElementList *global, char *element_name,
                         char *new_element_reference) {
    Element *temp = get_element_by_reference(global, &element_name);
    temp->ref = (char*) UA_realloc(temp->ref, (strlen(new_element_reference)+1)*sizeof(char));
    memset(temp->ref, 0, (strlen(new_element_reference)+1)*sizeof(char));
    strcpy(temp->ref, new_element_reference);
    free(new_element_reference);
    free(element_name);
}

void
add_in_list_children(ElementList *global, Operand *oper) {
    Element *temp = get_last_element_from_list(global);
    temp->element.oper.nbr_children++;
    temp->element.oper.children = (Operand*)
        UA_realloc(temp->element.oper.children, temp->element.oper.nbr_children*sizeof(Operand));
    memcpy(&temp->element.oper.children[temp->element.oper.nbr_children-1], oper, sizeof(Operand));
}

void
create_in_list_operator(ElementList *global, Operand *oper, char *element_ref) {
    Element *temp = create_next_operator_element(global);
    save_string(element_ref, &temp->ref);
    temp->element.oper.filter = UA_FILTEROPERATOR_INLIST;
    free(element_ref);
    add_in_list_children(global, oper);
}

UA_Variant
parse_literal(char *yytext, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);

    void *data = UA_new(type);
    if(!data)
        return var;
    UA_String src = UA_STRING(yytext);
    UA_StatusCode res = UA_decodeJson(&src, data, type, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(data);
        return var;
    }

    UA_Variant_setScalar(&var, data, type);
    return var;
}
