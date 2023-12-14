/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */


#include "open62541/plugin/eventfilter_parser.h"


static UA_Parsed_Element_List *create_next_operator_element(UA_Element_List *elements){
    UA_Parsed_Element_List *element = (UA_Parsed_Element_List *) UA_calloc(1, sizeof(UA_Parsed_Element_List));
    element->identifier = parsed_operator;
    element->element.oper.ContentFilterArrayPosition =0;
    TAILQ_INSERT_TAIL(&elements->head, element, element_entries);
    return element;
}

void save_string(char *str, char **local_str){
    *local_str = (char*) UA_calloc(strlen(str)+1, sizeof(char));
    strcpy(*local_str, str);
}

void create_next_operand_element(UA_Element_List *elements, UA_Parsed_Operand *operand, char *ref){
    UA_Parsed_Element_List *element = (UA_Parsed_Element_List *) UA_calloc(1, sizeof(UA_Parsed_Element_List));
    element->identifier = parsed_operand;
    save_string(ref, &element->ref);
    free(ref);
    memcpy(&element->element.operand.identifier, &operand->identifier, sizeof(OperandIdentifier));
    if(operand->identifier == extensionobject){
        UA_ExtensionObject_copy(&operand->value.extension, &element->element.operand.value.extension);
        UA_ExtensionObject_clear(&operand->value.extension);
    }
    else{
        save_string(operand->value.element_ref, &element->element.operand.value.element_ref);
        free(operand->value.element_ref);
    }
    TAILQ_INSERT_TAIL(&elements->head, element, element_entries);
}

static UA_Parsed_Element_List* get_element_by_reference(UA_Element_List *elements, char **reference){
    UA_Parsed_Element_List *temp;
    TAILQ_FOREACH(temp, &elements->head, element_entries){
        if(strcmp(temp->ref, *reference)==0)
            return temp;
    }
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to find the element with reference: %s", *reference);
    return temp;
}

static UA_Parsed_Element_List* get_element_by_idx_position(UA_Element_List *elements, size_t idx){
    UA_Parsed_Element_List *temp;
    TAILQ_FOREACH(temp, &elements->head, element_entries){
        if(temp->element.oper.ContentFilterArrayPosition == idx)
            return temp;
    }
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to find the element with idx %" PRIu64, idx);
    return temp;
}

static UA_Parsed_Element_List *get_last_element_from_list(UA_Element_List *elements){
    return TAILQ_LAST(&elements->head, parsed_filter_elements);
}

static UA_StatusCode solve_children_references(UA_Parsed_Element_List *temp, UA_Element_List *elements, size_t *position, size_t *nbr_children, char ***child_references){
    for(size_t i=0; i< temp->element.oper.nbr_children; i++){
        if(temp->element.oper.children[i].identifier == elementoperand){
            UA_Parsed_Element_List *temp_1 = get_element_by_reference(elements, &temp->element.oper.children[i].value.element_ref);
            if(temp_1->identifier == parsed_operator){
                if(temp_1->element.oper.ContentFilterArrayPosition == 0){
                    (*nbr_children)++;
                    *child_references = (char**) UA_realloc(*child_references, *nbr_children*sizeof(char*));
                    save_string(temp->element.oper.children[i].value.element_ref, &(*child_references)[*nbr_children-1]);
                    (*position)++;
                    temp_1->element.oper.ContentFilterArrayPosition = *position;
                }
                free(temp->element.oper.children[i].value.element_ref);
                UA_ExtensionObject extension;
                extension.encoding = UA_EXTENSIONOBJECT_DECODED;
                extension.content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
                extension.content.decoded.data = &temp_1->element.oper.ContentFilterArrayPosition;
                UA_ExtensionObject_copy(&extension, &temp->element.oper.children[i].value.extension);
                temp->element.oper.children[i].identifier = extensionobject;
            }
            else{
                if(temp_1->element.operand.identifier == elementoperand){
                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error! Unsolved reference to an ElementOperand");
                    return UA_STATUSCODE_BAD;
                }
                free(temp->element.oper.children[i].value.element_ref);
                temp->element.oper.children[i].identifier = extensionobject;
                UA_ExtensionObject_copy(&temp_1->element.operand.value.extension, &temp->element.oper.children[i].value.extension);

            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode check_recursion_on_operands(UA_Parsed_Element_List *element, size_t init_element, UA_Element_List *elements, size_t *ctr, UA_StatusCode *status){
    if(*status != UA_STATUSCODE_GOOD){
        return UA_STATUSCODE_GOOD;
    }
    for(size_t i=0; i<element->element.oper.nbr_children; i++){
        if(element->element.oper.children[i].value.extension.content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]){
            UA_ElementOperand *temp = (UA_ElementOperand*) element->element.oper.children[i].value.extension.content.decoded.data;
            UA_Parsed_Element_List *next_element = get_element_by_idx_position(elements, (size_t) temp->index);
            if(next_element->element.oper.ContentFilterArrayPosition == init_element){
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Operator %s has a loop to itself", next_element->ref);
                return UA_STATUSCODE_BAD;
            }
            //when checking an element and a loop exists inside the tree structure, a termination condition is required,
            //otherwise, the loop is detecter, however as it does not involve the current element, the condition based on the element index
            //will never be met
            if(*ctr > 1000){
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Checking Operator on position %." PRIu64 " Loop within the ContentFilter structure detected", init_element);
                break;
            }
            (*ctr)++;
            UA_StatusCode retval = check_recursion_on_operands(next_element, init_element, elements, ctr, status);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void handle_filter_structure(char *first_element, UA_Element_List *elements, size_t *position){
    UA_Parsed_Element_List *temp;
    size_t nbr_children = 0;
    char **children_list = (char**) UA_calloc(0, sizeof(char*));
    temp = get_element_by_reference(elements, &first_element);
    solve_children_references(temp, elements, position, &nbr_children, &children_list);
    while(nbr_children >0){
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
            UA_Parsed_Element_List *child = get_element_by_reference(elements, &temp_list[i]);
            solve_children_references(child, elements, position, &nbr_children, &children_list);
            free(temp_list[i]);
        }
        free(temp_list);
    }
    free(children_list);
    free(first_element);
}

static UA_StatusCode solve_operand_references(UA_Element_List *elements){
    size_t current_nbr = 1, last_nbr = 0;
    UA_Parsed_Element_List *temp, *temp1;
    while(current_nbr > 0){
        current_nbr = 0;
        TAILQ_FOREACH(temp, &elements->head, element_entries){
            if(temp->identifier == parsed_operand){
                if(temp->element.operand.identifier == elementoperand){
                    temp1 = get_element_by_reference(elements, &temp->element.operand.value.element_ref);
                    if(temp1->identifier == parsed_operator){
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error! An operand references an Operator");
                        return UA_STATUSCODE_BAD;
                    }

                    if(temp1->element.operand.identifier == elementoperand){
                        current_nbr++;
                    }
                    else{
                        free(temp->element.operand.value.element_ref);
                        temp->element.operand.identifier = extensionobject;
                        UA_ExtensionObject_copy(&temp1->element.operand.value.extension, &temp->element.operand.value.extension);
                    }
                }
            }
        }
        if(current_nbr == last_nbr && last_nbr != 0){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error! Unable to solve all operand references");
            return UA_STATUSCODE_BAD;
        }
        last_nbr = current_nbr;
    }
    return UA_STATUSCODE_GOOD;
}

static void clear_operand_list(UA_Parsed_Operand *operand_list, size_t list_size){
    for(size_t i=0; i< list_size; i++){
        operand_list[i].identifier == elementoperand ? free(operand_list[i].value.element_ref):UA_ExtensionObject_clear(&operand_list[i].value.extension);
    }
}

static void clear_linked_list(UA_Element_List *elements){
    UA_Parsed_Element_List *temp, *temp1;
    TAILQ_FOREACH_SAFE(temp, &elements->head, element_entries, temp1){
        free(temp->ref);
        if(temp->identifier == parsed_operand){
            clear_operand_list(&temp->element.operand, 1);
        }
        else{
            UA_FilterOperator_clear(&temp->element.oper.filter);
            clear_operand_list(temp->element.oper.children, temp->element.oper.nbr_children);
            free(temp->element.oper.children);
        }
        TAILQ_REMOVE(&elements->head, temp, element_entries);
        free(temp);
    }
}

static void add_content_filter_element(UA_ContentFilterElement *filterElement, UA_Parsed_Operator *parsedElement){
    UA_ContentFilterElement_init(filterElement);
    filterElement->filterOperandsSize = parsedElement->nbr_children;
    filterElement->filterOperands = (UA_ExtensionObject*) UA_calloc(parsedElement->nbr_children, sizeof(UA_ExtensionObject));
    filterElement->filterOperator = parsedElement->filter;
    for(size_t i=0; i< parsedElement->nbr_children; i++){
        UA_ExtensionObject_copy(&parsedElement->children[i].value.extension, &filterElement->filterOperands[i]);
        UA_ExtensionObject_clear(&parsedElement->children[i].value.extension);
    }
}

UA_StatusCode create_content_filter(UA_Element_List *elements, UA_ContentFilter *filter, char *first_element, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD){
        clear_linked_list(elements);
        free(first_element);
        return status;
    }
    size_t position = 0;
    UA_StatusCode retval = solve_operand_references(elements);
    if(retval!= UA_STATUSCODE_GOOD){
        clear_linked_list(elements);
        free(first_element);
        return retval;
    }
    handle_filter_structure(first_element, elements, &position);
    ////check for loops inside the tree structure
    UA_Parsed_Element_List *temp;
    TAILQ_FOREACH(temp, &elements->head, element_entries){
        size_t ctr = 0;
        retval = check_recursion_on_operands(temp, temp->element.oper.ContentFilterArrayPosition, elements, &ctr, &status);
        if(retval != UA_STATUSCODE_GOOD){
            clear_linked_list(elements);
            return retval;
        }
    }
    //add the elements to the contentFilter based on their array position values
    filter->elementsSize = position+1;
    filter->elements = (UA_ContentFilterElement*) UA_Array_new(filter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
    for(size_t i=0; i<=position; i++){
        temp = get_element_by_idx_position(elements, i);
        add_content_filter_element(&filter->elements[i], &temp->element.oper);
    }
    clear_linked_list(elements);
    return UA_STATUSCODE_GOOD;
}

static void copy_children(UA_Parsed_Operator element, UA_Parsed_Element_List *temp){
    for(size_t i=0; i<element.nbr_children; i++){
        if(element.children[i].identifier == elementoperand){
            temp->element.oper.children[i].identifier = elementoperand;
            save_string(element.children[i].value.element_ref, &temp->element.oper.children[i].value.element_ref);
        }
        else{
            temp->element.oper.children[i].identifier = extensionobject;
            UA_ExtensionObject_copy(&element.children[i].value.extension, &temp->element.oper.children[i].value.extension);
        }
    }
}

static void add_operator_children(UA_Element_List *global, UA_Parsed_Operator element){
    UA_Parsed_Element_List *temp = get_last_element_from_list(global);
    memcpy(&temp->element.oper.nbr_children, &element.nbr_children, sizeof(size_t));
    temp->element.oper.children = (UA_Parsed_Operand*) UA_calloc(element.nbr_children, sizeof(UA_Parsed_Operand));
    UA_FilterOperator_copy(&element.filter, &temp->element.oper.filter);
    copy_children(element, temp);
}

void add_new_operator(UA_Element_List *global, char *operator_ref, UA_Parsed_Operator *element){
    UA_Parsed_Element_List *temp = create_next_operator_element(global);
    save_string(operator_ref, &temp->ref);
    free(operator_ref);
    add_operator_children(global, *element);
    clear_operand_list(element->children, element->nbr_children);
    free(element->children);
}

static void check_SAO(UA_SimpleAttributeOperand *sao){
    if(sao->typeDefinitionId.identifier.numeric == 0 && sao->typeDefinitionId.identifierType == UA_NODEIDTYPE_NUMERIC){
        sao->typeDefinitionId = UA_NODEID_NUMERIC(0, 2041);}
    if(sao->attributeId == 0){sao->attributeId = 13;}
    if(sao->attributeId == 1){UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Extracting a NodeId from a SimpleAttributeOperand is not supported in the current implementation");}
    //if(sao->indexRange.length == 0){sao->indexRange =UA_String_fromChars("");}
}

UA_StatusCode append_select_clauses(UA_SimpleAttributeOperand **select_clauses, size_t *sao_size, UA_ExtensionObject *extension, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(extension);
        return status;
    }
    if(*sao_size == 0){
        *sao_size = 1;
        *select_clauses = (UA_SimpleAttributeOperand*) UA_Array_new(*sao_size, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        UA_SimpleAttributeOperand_init(*select_clauses);
        UA_SimpleAttributeOperand_copy((UA_SimpleAttributeOperand*) extension->content.decoded.data, select_clauses[0]);
    }
    else{
        UA_StatusCode ret_val = UA_Array_append((void **)select_clauses, sao_size,(UA_SimpleAttributeOperand*) extension->content.decoded.data, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        if (ret_val != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to increase the select clauses %s", UA_StatusCode_name(ret_val));
            UA_ExtensionObject_clear(extension);
            return ret_val;
        }
    }
    UA_ExtensionObject_clear(extension);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode set_up_browsepath(UA_QualifiedName **q_name_list, size_t *size, char *str, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_RelativePath *path = UA_RelativePath_new();
    UA_String parsed_string = UA_String_fromChars(str);
    UA_StatusCode ret_val = UA_RelativePath_parse(path, parsed_string);
    if(ret_val != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to parse the RelativePath %s", UA_StatusCode_name(ret_val));
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

UA_StatusCode create_literal_operand(char *string, UA_LiteralOperand *lit, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_ByteString input_val = UA_String_fromChars(string);
    UA_StatusCode ret_val = UA_decodeJson(&input_val, (void *)&lit->value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(ret_val != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to decode the Json %s", UA_StatusCode_name(ret_val));
        UA_ByteString_clear(&input_val);
        return ret_val;
    }
    UA_ByteString_clear(&input_val);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode create_nodeId_from_string(char *identifier, UA_NodeId *id, UA_StatusCode status){
    if( status != UA_STATUSCODE_GOOD) {
        free(identifier);
        return status;
    }
    UA_String str = UA_String_fromChars(identifier);
    free(identifier);
    UA_NodeId_init(id);
    UA_StatusCode ret_val = UA_NodeId_parse(id, str);
    if(ret_val != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to parse the NodeId %s", UA_StatusCode_name(ret_val));
        UA_String_clear(&str);
        return ret_val;
    }
    UA_String_clear(&str);
    return UA_STATUSCODE_GOOD;
}

void handle_elementoperand(UA_Parsed_Operand *operand, char *ref){
    memset(operand, 0, sizeof(UA_Parsed_Operand));
    operand->identifier = elementoperand;
    save_string(ref, &operand->value.element_ref);
}

void handle_sao(UA_SimpleAttributeOperand *simple, UA_Parsed_Operand *operand){
    UA_SimpleAttributeOperand *sao = UA_SimpleAttributeOperand_new();
    UA_SimpleAttributeOperand_copy(simple, sao);
    for(size_t i=0; i< simple->browsePathSize; i++){
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
    operand->identifier = extensionobject;
}

static void create_element_reference(size_t *branch_nbr, char **ref, char *ref_identifier){
    char ref_nbr[128];
    sprintf(ref_nbr, "%zu" PRIu64 , *branch_nbr);
    *ref = (char*) UA_calloc(strlen(ref_identifier)+1+strlen(ref_nbr), sizeof(char));
    strcpy(*ref, ref_identifier);
    strcat(*ref, ref_nbr);
    (*branch_nbr)++;
}

static void handle_branch_operator(UA_Parsed_Element_List *element, UA_FilterOperator oper, size_t *branch_ctr){
    element->element.oper.filter = oper;
    create_element_reference(branch_ctr, &element->ref, "operator_reference_");
}

void add_operand_from_branch(char **ref, size_t *operand_ctr, UA_Parsed_Operand *operand, UA_Element_List *global){
    char *temp, temp1[100];
    create_element_reference(operand_ctr, &temp, "operand_reference_");
    strcpy(temp1, temp);
    create_next_operand_element(global, operand, temp);
    save_string(temp1, ref);
}

static void add_child_references(UA_Parsed_Element_List *element, char **ref_1, char **ref_2){
    element->element.oper.nbr_children = 2;
    element->element.oper.children = (UA_Parsed_Operand *) UA_calloc(2, sizeof(UA_Parsed_Operand));
    for(size_t i=0; i<2; i++){
        element->element.oper.children[i].identifier = elementoperand;
        i == 0 ? save_string(*ref_1, &element->element.oper.children[0].value.element_ref)
               : save_string(*ref_2, &element->element.oper.children[1].value.element_ref);
    }
}

static UA_NodeId copy_nodeId(UA_NodeId *id){
    UA_NodeId temp;
    UA_NodeId_copy(id, &temp);
    UA_NodeId_clear(id);
    return temp;
}

static void set_up_ext_object_from_literal(UA_ExtensionObject *obj, UA_LiteralOperand *value){
    UA_LiteralOperand *lit = UA_LiteralOperand_new();
    UA_LiteralOperand_copy(value, lit);
    UA_LiteralOperand_clear(value);
    UA_ExtensionObject_init(obj);
    obj->encoding = UA_EXTENSIONOBJECT_DECODED;
    obj->content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    obj->content.decoded.data = lit;
}

void set_up_variant_from_nodeId(UA_NodeId *id, UA_Variant *litvalue){
    UA_NodeId temp = *id;
    UA_Variant_setScalarCopy(litvalue, &temp, &UA_TYPES[UA_TYPES_NODEID]);
}

void create_nodeid_element(UA_Element_List *elements, UA_NodeId *id, char *ref){
    UA_Parsed_Operand operand;
    memset(&operand, 0, sizeof(UA_Parsed_Operand));
    UA_LiteralOperand lit;
    set_up_variant_from_nodeId(id, &lit.value);
    UA_NodeId_clear(id);
    set_up_ext_object_from_literal(&operand.value.extension, &lit);
    operand.identifier = extensionobject;
    create_next_operand_element(elements, &operand, ref);
}

void handle_oftype_nodeId(UA_Parsed_Operator *element, UA_NodeId *id){
    UA_FilterOperator_init(&element->filter);
    element->filter = UA_FILTEROPERATOR_OFTYPE;
    element->nbr_children = 1;
    element->children = (UA_Parsed_Operand*) UA_calloc(1, sizeof(UA_Parsed_Operand));
    UA_LiteralOperand lit;
    set_up_variant_from_nodeId(id, &lit.value);
    UA_NodeId_clear(id);
    set_up_ext_object_from_literal(&element->children[0].value.extension, &lit);
    element->children[0].identifier = extensionobject;
}

void handle_literal_operand(UA_Parsed_Operand *operand, UA_LiteralOperand *literalValue){
    set_up_ext_object_from_literal(&operand->value.extension, literalValue);
    operand->identifier = extensionobject;
    UA_LiteralOperand_clear(literalValue);
}

void set_up_typeid(UA_Local_Operand *operand){
    UA_NodeId temp = copy_nodeId(&operand->id);
    operand->sao.typeDefinitionId = copy_nodeId(&temp);
}

void add_child_operands(UA_Parsed_Operand *operand_list, size_t operand_list_size, UA_Parsed_Operator *element, UA_FilterOperator oper){
    element->nbr_children = operand_list_size;
    element->filter = oper;
    element->children = (UA_Parsed_Operand*) UA_calloc(operand_list_size, sizeof(UA_Parsed_Operand));
    for(size_t i=0; i< operand_list_size; i++){
        if(operand_list[i].identifier == elementoperand){
            save_string(operand_list[i].value.element_ref, &element->children[i].value.element_ref);
            element->children[i].identifier = elementoperand;
        }else{
            UA_ExtensionObject_copy(&operand_list[i].value.extension, &element->children[i].value.extension);
            element->children[i].identifier = extensionobject;
            UA_ExtensionObject_clear(&operand_list[i].value.extension);
        }
    }
}

void handle_between_operator(UA_Parsed_Operator *element, UA_Parsed_Operand *operand_1, UA_Parsed_Operand *operand_2, UA_Parsed_Operand *operand_3){
    UA_Parsed_Operand operand_list[3] = {*operand_1, *operand_2, *operand_3};
    add_child_operands(operand_list, 3, element, UA_FILTEROPERATOR_BETWEEN);
    clear_operand_list(operand_list, 3);
}

void handle_two_operands_operator(UA_Parsed_Operator *element, UA_Parsed_Operand *operand_1, UA_Parsed_Operand *operand_2, UA_FilterOperator *filter){
    UA_Parsed_Operand operand_list[2] = {*operand_1, *operand_2};
    add_child_operands(operand_list, 2, element, *filter);
    clear_operand_list(operand_list, 2);
}

void init_item_list(UA_Element_List *global, UA_Counters *ctr){
    TAILQ_INIT(&global->head);
    ctr->branch_element_number = 0;
    ctr->for_operator_reference = 0;
    ctr->operand_ctr = 0;
}

void create_branch_element(UA_Element_List *global, size_t *branch_element_number, UA_FilterOperator filteroperator, char *ref_1, char *ref_2, char **ref){
    UA_Parsed_Element_List *temp = create_next_operator_element(global);
    handle_branch_operator(temp, filteroperator, branch_element_number);
    add_child_references(temp, &ref_1, &ref_2);
    save_string(temp->ref, ref);
    free(ref_1);
    free(ref_2);
}

void handle_for_operator(UA_Element_List *global, size_t *for_operator_reference, char **ref, UA_Parsed_Operator *element){
    UA_Parsed_Element_List *temp = create_next_operator_element(global);
    create_element_reference(for_operator_reference, &temp->ref, "for_reference_");
    memcpy(&temp->element.oper.nbr_children, &element->nbr_children, sizeof(size_t));
    temp->element.oper.filter = element->filter;
    temp->element.oper.children = (UA_Parsed_Operand*) UA_calloc(element->nbr_children, sizeof(UA_Parsed_Operand));
    copy_children(*element, temp);
    save_string(temp->ref, ref);
    clear_operand_list(element->children, element->nbr_children);
    free(element->children);
}

void change_element_reference(UA_Element_List *global, char *element_name, char *new_element_reference){
    UA_Parsed_Element_List *temp = get_element_by_reference(global, &element_name);
    temp->ref = (char*) UA_realloc(temp->ref, (strlen(new_element_reference)+1)*sizeof(char));
    memset(temp->ref, 0, (strlen(new_element_reference)+1)*sizeof(char));
    strcpy(temp->ref, new_element_reference);
    free(new_element_reference);
    free(element_name);
}

void add_in_list_children(UA_Element_List *global, UA_Parsed_Operand *oper){
    UA_Parsed_Element_List *temp = get_last_element_from_list(global);
    temp->element.oper.nbr_children++;
    temp->element.oper.children = (UA_Parsed_Operand*) UA_realloc(temp->element.oper.children, temp->element.oper.nbr_children*sizeof(UA_Parsed_Operand));
    memcpy(&temp->element.oper.children[temp->element.oper.nbr_children-1], oper, sizeof(UA_Parsed_Operand));
}

void create_in_list_operator(UA_Element_List *global, UA_Parsed_Operand *oper, char *element_ref){
    UA_Parsed_Element_List *temp = create_next_operator_element(global);
    save_string(element_ref, &temp->ref);
    temp->element.oper.filter = UA_FILTEROPERATOR_INLIST;
    free(element_ref);
    add_in_list_children(global, oper);
}

void append_string(char **string, char *yytext){
    size_t old_size = (*string)? strlen(*string):0;
    size_t new_size = old_size+strlen(yytext)+1;
    *string = (char *)UA_realloc(*string, new_size*sizeof(char));
    memset((void*)((char*)(*string) + old_size),0, (new_size-old_size)*sizeof(char));
    strcat(*string, yytext);
}

void set_up_variant_from_bool(char *yytext, UA_Variant *litvalue){
    UA_Boolean val;
    if(strcmp(yytext, "false") == 0 || strcmp(yytext, "False") == 0 || strcmp(yytext, "0") == 0){
        val = false;
        UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    else if(strcmp(yytext, "true") == 0 || strcmp(yytext, "True") == 0 || strcmp(yytext, "1") == 0){
        val = true;
        UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
}

void set_up_variant_from_string(char *yytext, UA_Variant *litvalue){
    UA_String val = UA_String_fromChars(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&val);
}

void set_up_variant_from_bstring(char *yytext, UA_Variant *litvalue){
    UA_ByteString val = UA_BYTESTRING(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BYTESTRING]);
}

void set_up_variant_from_float(char *yytext, UA_Variant *litvalue){
    UA_Float val = (UA_Float) strtod(yytext, NULL);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_FLOAT]);
}
void set_up_variant_from_double(char *yytext, UA_Variant *litvalue){
    UA_Double val = (UA_Double) strtod(yytext, NULL);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
}

void set_up_variant_from_sbyte(char *yytext, UA_Variant *litvalue){
    UA_SByte val = (UA_SByte) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_SBYTE]);
}

void set_up_variant_from_statuscode(char *yytext, UA_Variant *litvalue){
    UA_StatusCode val = (uint32_t) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode set_up_variant_from_expnodeid(char *yytext, UA_Variant *litvalue, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_ExpandedNodeId val;
    UA_ExpandedNodeId_init(&val);
    UA_String temp = UA_String_fromChars(yytext);
    UA_StatusCode retval = UA_ExpandedNodeId_parse(&val, temp);
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to parse the ExpandedNodeId %s", UA_StatusCode_name(retval));
        UA_ExpandedNodeId_clear(&val);
        UA_String_clear(&temp);
        return retval;
    }
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    UA_ExpandedNodeId_clear(&val);
    UA_String_clear(&temp);
    return UA_STATUSCODE_GOOD;
}

void set_up_variant_from_time(const char *yytext, UA_Variant *litvalue){
    UA_DateTimeStruct dt;
    memset(&dt, 0, sizeof(UA_DateTimeStruct));
    sscanf(yytext, "%hi-%hu-%huT%hu:%hu:%huZ",
           &dt.year, &dt.month, &dt.day, &dt.hour, &dt.min, &dt.sec);
    UA_DateTime val = UA_DateTime_fromStruct(dt);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_DATETIME]);
}

void set_up_variant_from_byte(char *yytext, UA_Variant *litvalue){
    UA_Byte val = (UA_Byte) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BYTE]);
}

UA_StatusCode set_up_variant_from_qname(char *str, UA_Variant *litvalue, UA_StatusCode status){
    if(status != UA_STATUSCODE_GOOD)
        return status;
    UA_RelativePath path;
    UA_String parsed_string = UA_String_fromChars(str);
    UA_StatusCode ret_val = UA_RelativePath_parse(&path, parsed_string);
    if(ret_val != UA_STATUSCODE_GOOD){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to parse the RelativePath %s", UA_StatusCode_name(ret_val));
        UA_String_clear(&parsed_string);
        UA_RelativePath_clear(&path);
        return ret_val;
    }
    UA_Variant_setScalarCopy(litvalue, &path.elements[0].targetName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    UA_String_clear(&parsed_string);
    UA_RelativePath_clear(&path);
    return UA_STATUSCODE_GOOD;
}

void set_up_variant_from_guid(char *yytext, UA_Variant *litvalue){
    UA_Guid val;
    UA_String str = UA_String_fromChars(yytext);
    UA_Guid_parse(&val, str);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_GUID]);
    UA_String_clear(&str);
}

void set_up_variant_from_int64(char *yytext, UA_Variant *litvalue){
    UA_Int64 val = (UA_Int64) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT64]);
}

void set_up_variant_from_localized(char *yytext, UA_Variant *litvalue){
    UA_LocalizedText val = UA_LOCALIZEDTEXT("en-us", yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
}

void set_up_variant_from_uint16(char *yytext, UA_Variant *litvalue){
    UA_UInt16 val = (UA_UInt16) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_UINT16]);
}

void set_up_variant_from_uint32(char *yytext, UA_Variant *litvalue){
    UA_UInt32 val = (UA_UInt32) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_UINT32]);
}

void set_up_variant_from_uint64(char *yytext, UA_Variant *litvalue){
    UA_UInt64 val = (UA_UInt64) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT64]);
}

void set_up_variant_from_int16(char *yytext, UA_Variant *litvalue){
    UA_Int16 val = (UA_Int16) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT16]);
}

void set_up_variant_from_int32(char *yytext, UA_Variant *litvalue){
    UA_Int32 val = (UA_Int32) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT32]);
}
