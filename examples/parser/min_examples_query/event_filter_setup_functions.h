//
// Created by flo47663 on 15.12.2022.
//
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <../deps/cj5.h>
#include "../deps/open62541_queue.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>

#ifndef OPEN62541_NODEID_SETUP_FUNCTIONS_H
#define OPEN62541_NODEID_SETUP_FUNCTIONS_H

#include <inttypes.h>

enum UA_VALUE_IDENTIFIER {
    DEFAULT    = 0,
    DATATYPE_SAO     = 1,
    DATATYPE_NODEID       = 2,
    DATATYPE_STRING = 3,
    DATATYPE_VARIANT = 4
};




typedef struct local {
    enum UA_VALUE_IDENTIFIER identifier;
    union{
        UA_SimpleAttributeOperand sao;
        UA_NodeId id;
        char *str;
        UA_Variant var;
    } value;
} UA_Local_Context;

typedef struct ParseElem {
    enum UA_VALUE_IDENTIFIER identifier;
    union {
        UA_SimpleAttributeOperand saoOperand;
        UA_NodeId nodeIdLiteralOperand;
        UA_Variant literalOperand;
        char *operandReference;
        struct {
            char *reference;
            int operatorType;
            size_t operandsSize;
            struct ParseElem *operands;
        } child_operator;
        char *tmpStr;
    } value;
} ParseElem;

enum OPERANDIDENTIFIER{
    ELEMENTOPERAND = 0,
    ELSE = 1
};

enum CONTENTFILTER_ELEMENT_IDENTIFIER{
    FILTERELEMENT = 0,
    BRANCHELEMENT = 1
};

typedef struct OperandReference{
    enum OPERANDIDENTIFIER identifier;
    union{
        //store the reference for elementoperands
        char *element_identifier;
        //store Literal and SAO
        UA_ExtensionObject extobj;
    }operand;
}UA_OperandReference;

typedef struct FilterElementContainer{
    UA_FilterOperator filter;
    size_t nbr_operands;
    UA_OperandReference *operand_list;
}UA_FilterElementContainer;

typedef struct filter_element{
    UA_FilterElementContainer element;
    char *ref;
}UA_filter_element;

typedef struct branch_element{
    UA_FilterOperator oper;
    size_t child_nbr;
    char *ref;
    char **child_ref;
}UA_branch_element;

typedef struct where_elements{
    enum CONTENTFILTER_ELEMENT_IDENTIFIER identifier;
    union{
        UA_filter_element filt;
        UA_branch_element branch;
    }element;
    TAILQ_ENTRY(where_elements) where_entries;
}UA_Where_Clause_List;

struct where_list{
    TAILQ_HEAD(, where_elements) where_clause_elements;
};

typedef struct select_SAO{
    TAILQ_ENTRY(select_SAO) select_entries;
    UA_SimpleAttributeOperand sao;
}UA_Select_Clauses_List;

typedef struct global_context {
    size_t br_ctr;
    size_t branch_element_number;
    int for_operator_reference;
    TAILQ_HEAD(, select_SAO) select_clauses_elements;
    struct where_list list;
    size_t filter_elements;
    UA_Where_Clause_List *children;
}UA_global_context;

static void append_filter_item(struct where_list filter_items, char *curr_identifier, char ***ref_idx_list, size_t *item_ctr){
    UA_Where_Clause_List *temp;
    TAILQ_FOREACH(temp, &filter_items.where_clause_elements, where_entries){
        if(temp->identifier == BRANCHELEMENT){
            if(strcmp(curr_identifier, temp->element.branch.ref)==0){
                //appent the children to the list
                for(size_t i=0; i< temp->element.branch.child_nbr; i++){
                    *ref_idx_list = (char**) UA_realloc(*ref_idx_list ,((*item_ctr)+1)*sizeof(char*));
                    (*item_ctr)++;
                    (*ref_idx_list)[(*item_ctr)-1] = (char*) UA_calloc(strlen(temp->element.branch.child_ref[i])+1, sizeof(char));
                    strcpy((*ref_idx_list)[*item_ctr-1], temp->element.branch.child_ref[i]);
                }
                for(size_t i=0; i< temp->element.branch.child_nbr; i++){
                    if(strlen(temp->element.branch.child_ref[i]) > 17){
                        char *branch = "branch_reference_";
                        char temp_char[18];
                        memcpy(temp_char, temp->element.branch.child_ref[i], 17*sizeof(char));
                        temp_char[17] = '\0';
                        if(strcmp(temp_char, branch)==0){
                            append_filter_item(filter_items, temp->element.branch.child_ref[i], ref_idx_list, item_ctr);
                        }
                    }
                }
            }
        }
    }
}

static UA_ContentFilter create_content_filter(struct where_list filter_items){
    //printf("create the content filter \n");
    char **ref_idx_list = (char**) UA_calloc(0, sizeof(char*));
    size_t item_ctr = 0;
    //TAILQ_FOREACH
    UA_ContentFilter filter;// = UA_ContentFilter_new();
    UA_ContentFilter_init(&filter);
    /*1. append a list with all branch elements and their children and build the structure -> use reference for the list entry -> idx = list position
     * search all children for each item */
    /* 2. build elementoperands*/
    /* 3. build contentfilter -> insert the filterelements, build from the linked list, into the idx list*/
    size_t list_ctr = 0;
    //count the number of list elements
    UA_Where_Clause_List *ctr;
    //printf("print out the linked list\n");
    TAILQ_FOREACH(ctr, &filter_items.where_clause_elements, where_entries){
        list_ctr++;
        if(ctr->identifier == BRANCHELEMENT){
            printf("branch %s \n", ctr->element.branch.ref);
            for(size_t i=0; i< ctr->element.branch.child_nbr; i++){
                printf("child %zu is %s\n", i, ctr->element.branch.child_ref[i]);
            }
            printf(" \n");
        }
        else{
            printf("filt %s \n", ctr->element.filt.ref);
            for(size_t i=0; i< ctr->element.filt.element.nbr_operands; i++){
                //printf()
                if(ctr->element.filt.element.operand_list[i].identifier == ELEMENTOPERAND){
                    printf("elementoperand with ref %s\n", ctr->element.filt.element.operand_list[i].operand.element_identifier);
                }
                else{
                    UA_String out = UA_STRING_NULL;
                    UA_print(&ctr->element.filt.element.operand_list[i].operand.extobj, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &out);
                    printf("%.*s\n", (int)out.length, out.data);
                    UA_String_clear(&out);
                }
            }
            printf(" \n");
        }
    }
    printf("the linked list has %zu elements \n", list_ctr);

    if(list_ctr > 2){
        UA_Where_Clause_List *temp;
        bool single_item_filter = true;
        char *item_zero = "branch_reference_0";
        TAILQ_FOREACH(temp, &filter_items.where_clause_elements, where_entries) {
            if(temp->identifier == BRANCHELEMENT) {
                if(strcmp(item_zero, temp->element.branch.ref) == 0) {
                    ref_idx_list = (char **)UA_realloc(ref_idx_list, (item_ctr + 1) * sizeof(char *));
                    ref_idx_list[0] = (char *)UA_calloc(strlen(temp->element.branch.ref) + 1, sizeof(char));
                    strcpy(ref_idx_list[0], temp->element.branch.ref);
                    item_ctr++;
                    single_item_filter = false;
                    //append the children to the list
                    for(size_t i = 0; i < temp->element.branch.child_nbr; i++) {
                        //add the element to the idx ref list
                        ref_idx_list = (char **)UA_realloc(ref_idx_list, (item_ctr + 1) * sizeof(char *));
                        item_ctr++;
                        ref_idx_list[item_ctr - 1] = (char *)UA_calloc(strlen(temp->element.branch.child_ref[i]) + 1,
                                                                       sizeof(char));
                        strcpy(ref_idx_list[item_ctr - 1], temp->element.branch.child_ref[i]);
                        //search the next branch element in case it exists
                    }
                    for(size_t i = 0; i < temp->element.branch.child_nbr; i++) {
                        if(strlen(temp->element.branch.child_ref[i]) > 17) {
                            char *branch = "branch_reference_";
                            char temp_char[18];
                            memcpy(temp_char, temp->element.branch.child_ref[i], 17 * sizeof(char));
                            temp_char[17] = '\0';
                            if(strcmp(temp_char, branch) == 0) {
                                append_filter_item(filter_items, temp->element.branch.child_ref[i], &ref_idx_list,
                                                   &item_ctr);
                            }
                        }
                    }
                }
            }
        }
        //int *used_list = (int*) UA_calloc(item_ctr, sizeof(int));
        for(size_t i = 0; i < item_ctr; i++) {
            printf("the filterelement on pos %zu has the reference %s \n", i, ref_idx_list[i]);
            //used_list[i] = 0;
        }
        if(single_item_filter == true) {
            printf("no branch elements \n");
        }

        //find the reference idx and create the element operands
        TAILQ_FOREACH(temp, &filter_items.where_clause_elements, where_entries) {
            if(temp->identifier == FILTERELEMENT) {
                for(size_t i = 0; i < temp->element.filt.element.nbr_operands; i++) {
                    if(temp->element.filt.element.operand_list[i].identifier == ELEMENTOPERAND) {
                        char ref[512];
                        int index;
                        strcpy(ref, temp->element.filt.element.operand_list[i].operand.element_identifier);
                        //search the identifier in the ref_idx_list
                        bool element_operand_ref_found = false;
                        for(size_t j = 0; j < item_ctr; j++) {
                            if(strcmp(ref_idx_list[j], ref) == 0) {
                                element_operand_ref_found = true;
                                index = (int)j;
                            }
                        }
                        if(element_operand_ref_found == true) {
                            free(temp->element.filt.element.operand_list[i].operand.element_identifier);
                            //reset the union of the element operand container and create the extensionobject
                            //memset(temp->element.filt.element.operand_list[i].operand.element_identifier, 0,
                            //       sizeof(char));
                            temp->element.filt.element.operand_list[i].identifier = ELSE;
                            UA_ElementOperand *element = UA_ElementOperand_new();
                            element->index = index;
                            UA_ExtensionObject ext;
                            UA_ExtensionObject_init(&ext);
                            ext.encoding = UA_EXTENSIONOBJECT_DECODED;
                            ext.content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
                            ext.content.decoded.data = element;
                            UA_ExtensionObject_copy(&ext, &temp->element.filt.element.operand_list[i].operand.extobj);
                        } else {
                            printf("the reference %s of the element operand does not exists \n",
                                   temp->element.filt.element.operand_list[i].operand.element_identifier);
                        }

                    }
                }
            }
        }
        //create the content filter branch
        filter.elementsSize = item_ctr;
        filter.elements = (UA_ContentFilterElement *)UA_calloc(item_ctr, sizeof(UA_ContentFilterElement));
        for(size_t i = 0; i < item_ctr; i++) {
            //create the contentfilterelement
            UA_ContentFilterElement element;
            UA_ContentFilterElement_init(&element);
            TAILQ_FOREACH(temp, &filter_items.where_clause_elements, where_entries) {
                //find the contentfilterelement based on its reference
                if(temp->identifier == BRANCHELEMENT) {
                    //printf("the element is a branch element with ref %s \n", temp->element.branch.ref);
                    if(strcmp(temp->element.branch.ref, ref_idx_list[i]) == 0) {
                        element.filterOperator = temp->element.branch.oper;
                        //branch elements have always two operands
                        element.filterOperandsSize = 2;
                        element.filterOperands = (UA_ExtensionObject *)UA_calloc(2, sizeof(UA_ExtensionObject));
                        for(size_t z = 0; z < 2; z++) {
                            int index;
                            for(size_t j = 0; j < item_ctr; j++) {
                                //char *test = temp->element.branch.child_ref[z];
                                //strcpy(test, temp->element.branch.child_ref[z]);
                                //printf("required %s \n", test);
                                //char *test_char = ref_idx_list[j];
                                //printf("curr list %s \n", test_char);
                                bool found = false;
                                if(strcmp(ref_idx_list[j], temp->element.branch.child_ref[z]) ==0 /*&& used_list[j] == 0*/) {
                                    index = (int)j;
                                    found = true;
                                    //used_list[j] = 1;
                                }
                                UA_ElementOperand *oper = UA_ElementOperand_new();
                                oper->index = index;
                                UA_ExtensionObject_init(&element.filterOperands[z]);
                                element.filterOperands[z].encoding = UA_EXTENSIONOBJECT_DECODED;
                                element.filterOperands[z].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
                                element.filterOperands[z].content.decoded.data = oper;
                                if (found == true){
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    //printf("the element is a branch element with ref %s \n", temp->element.filt.ref);
                    if(strcmp(temp->element.filt.ref, ref_idx_list[i]) == 0) {
                        element.filterOperator = temp->element.filt.element.filter;
                        element.filterOperandsSize = temp->element.filt.element.nbr_operands;
                        element.filterOperands = (UA_ExtensionObject *)UA_calloc(
                            temp->element.filt.element.nbr_operands, sizeof(UA_ExtensionObject));
                        for(size_t j = 0; j < temp->element.filt.element.nbr_operands; j++) {
                            UA_ExtensionObject_init(&element.filterOperands[j]);
                            UA_ExtensionObject_copy(&temp->element.filt.element.operand_list[j].operand.extobj,
                                                    &element.filterOperands[j]);
                        }
                    }
                }
            }
            UA_ContentFilterElement_copy(&element, &filter.elements[i]);
            //free the memory of the elements
            for(size_t j=0; j< element.filterOperandsSize; j++){
                memset(&element.filterOperands[j].content.decoded.data, 0, sizeof(void*));
                UA_ExtensionObject_clear(&element.filterOperands[j]);
            }
            //free(element.filterOperands);
            UA_ContentFilterElement_clear(&element);
        }
    }
    else if(list_ctr ==1){
        printf("case single for operator and no for clause \n");

        filter.elementsSize = 1;
        //create the filterelement
        filter.elements = (UA_ContentFilterElement*) UA_calloc(1, sizeof(UA_ContentFilterElement));
        UA_Where_Clause_List *check;
        TAILQ_FOREACH(check, &filter_items.where_clause_elements, where_entries){
            if(check->identifier == BRANCHELEMENT){
                printf("the element is a branch element with ref %s \n", check->element.branch.ref);
            }
            else{
                printf("the element is a filter element with ref %s \n", check->element.filt.ref);
                UA_FilterOperator_copy(&check->element.filt.element.filter, &filter.elements[0].filterOperator);
                filter.elements[0].filterOperandsSize = check->element.filt.element.nbr_operands;
                filter.elements[0].filterOperands = (UA_ExtensionObject*) UA_calloc(check->element.filt.element.nbr_operands, sizeof(UA_ExtensionObject));
                for(size_t i=0; i < check->element.filt.element.nbr_operands; i++){
                    /*printf("copy extensionobjkect %zu \n", i);
                    UA_String out = UA_STRING_NULL;
                    UA_print(&check->element.filt.element.operand_list[i].operand.extobj, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &out);
                    printf("print the filteroperator %.*s\n", (int)out.length, out.data);
                    UA_String_clear(&out);
                            */
                    UA_ExtensionObject_copy(&check->element.filt.element.operand_list[i].operand.extobj, &filter.elements[0].filterOperands[i]);


                    /*
                    UA_print(&filter.elements[0].filterOperands[i], &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &out);
                    printf("print the filteroperator %.*s\n", (int)out.length, out.data);
                    UA_String_clear(&out);*/

                }
            }
        }
    }
    //free the linked list
    UA_Where_Clause_List *where_init, *where_fin;
    TAILQ_FOREACH_SAFE(where_init, &filter_items.where_clause_elements, where_entries, where_fin){
        //clear the memory of the different elements
        if(where_init->identifier == BRANCHELEMENT){
            free(where_init->element.branch.ref);
            for(size_t j=0; j< where_init->element.branch.child_nbr; j++){
                free(where_init->element.branch.child_ref[j]);
            }
            free(where_init->element.branch.child_ref);
        }
        else{
            free(where_init->element.filt.ref);
            for(size_t j=0; j< where_init->element.filt.element.nbr_operands; j++){
                UA_ExtensionObject_clear(&where_init->element.filt.element.operand_list[j].operand.extobj);
            }
            free(where_init->element.filt.element.operand_list);
        }
        TAILQ_REMOVE(&filter_items.where_clause_elements, where_init, where_entries);
        free(where_init);
    };



    //free the allocated memory
    for(size_t i=0; i< item_ctr; i++){
        free(ref_idx_list[i]);
    }
    free(ref_idx_list);

    /*UA_String out = UA_STRING_NULL;
    UA_print(filter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
    printf("print the filter%.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);*/

    return filter;

}

static void set_up_browsepath(UA_QualifiedName **q_name_list, size_t *size, char *yytext){
    UA_RelativePath *path = UA_RelativePath_new();
    UA_String parsed_string = UA_String_fromChars(yytext);
    UA_StatusCode retval = UA_RelativePath_parse(path, parsed_string);
    if(retval != UA_STATUSCODE_GOOD){
        printf("failed to parse the browsepath");
    }
    memcpy(size, &path->elementsSize, sizeof(size_t));
    *q_name_list = (UA_QualifiedName*) UA_calloc(*size, sizeof(UA_QualifiedName));
    for(size_t i=0; i< path->elementsSize; i++){
        UA_QualifiedName_copy(&path->elements[i].targetName, &(*q_name_list)[i]);
    }
    UA_RelativePath_delete(path);
    UA_String_clear(&parsed_string);
}

static void set_up_ext_object_from_sao(UA_ExtensionObject *obj, UA_SimpleAttributeOperand *value){
    UA_SimpleAttributeOperand *sao = UA_SimpleAttributeOperand_new();
    UA_SimpleAttributeOperand_copy(value, sao);

    UA_ExtensionObject_init(obj);
    obj->encoding = UA_EXTENSIONOBJECT_DECODED;
    obj->content.decoded.type = &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND];
    obj->content.decoded.data = sao;
}

static void set_up_ext_object_from_literal(UA_ExtensionObject *obj, UA_LiteralOperand *value){
    UA_LiteralOperand *lit = UA_LiteralOperand_new();
    UA_LiteralOperand_copy(value, lit);
    UA_LiteralOperand_clear(value);
    UA_ExtensionObject_init(obj);
    obj->encoding = UA_EXTENSIONOBJECT_DECODED;
    obj->content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    //memset(obj->content.decoded.data, 1, sizeof(UA_LiteralOperand));
    //memcpy(obj->content.decoded.data, (void*) value, sizeof(UA_LiteralOperand));
    obj->content.decoded.data = lit;
}

static void set_up_variant_from_bool(char *yytext, UA_Variant *litvalue){
    UA_Boolean val;
    printf("yytext has the value %s \n", yytext);
    if(strcmp(yytext, "false") == 0 || strcmp(yytext, "False") == 0 || strcmp(yytext, "0") == 0){
        val = false;
        UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    else if(strcmp(yytext, "true") == 0 || strcmp(yytext, "True") == 0 || strcmp(yytext, "1") == 0){
        val = true;
        UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    else{
        printf("wrong input value for boolean statement \n");
    }
}

static void set_up_variant_from_string(char *yytext, UA_Variant *litvalue){
    UA_String val = UA_String_fromChars(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_STRING]);
}

static void set_up_variant_from_bstring(char *yytext, UA_Variant *litvalue){
    UA_ByteString val = UA_BYTESTRING(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BYTESTRING]);
}

static void set_up_variant_from_float(char *yytext, UA_Variant *litvalue){
    UA_Float val = (UA_Float) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_FLOAT]);
}

static void set_up_variant_from_double(char *yytext, UA_Variant *litvalue){
    UA_Double val = (UA_Double) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
}

static void set_up_variant_from_sbyte(char *yytext, UA_Variant *litvalue){
    UA_SByte val = (UA_SByte) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_SBYTE]);
}

static void set_up_variant_from_statuscode(char *yytext, UA_Variant *litvalue){
    UA_StatusCode val = (uint32_t) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_STATUSCODE]);

}

static void set_up_variant_from_expnodeid(char *yytext, UA_Variant *litvalue){
    UA_ExpandedNodeId val;
    UA_ExpandedNodeId_init(&val);
    printf("the parsed expanded NodeId is %s \n", yytext);
    UA_String temp = UA_String_fromChars(yytext);
    UA_StatusCode retval = UA_ExpandedNodeId_parse(&val, temp);
    if(retval != UA_STATUSCODE_GOOD){
        printf("failed to parse the expanded NodeId with errorcode %s \n", UA_StatusCode_name(retval));
    }
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
}

static void set_up_variant_from_time(const char *yytext, UA_Variant *litvalue){
    UA_DateTimeStruct dt;
    memset(&dt, 0, sizeof(UA_DateTimeStruct));
    sscanf(yytext, "%hi-%hu-%huT%hu:%hu:%huZ",
           &dt.year, &dt.month, &dt.day, &dt.hour, &dt.min, &dt.sec);
    UA_DateTime val = UA_DateTime_fromStruct(dt);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_DATETIME]);
}

static void set_up_variant_from_byte(char *yytext, UA_Variant *litvalue){
    UA_Byte val = (UA_Byte) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_BYTE]);
}

static void set_up_variant_from_qname(char *yytext, UA_Variant *litvalue){
    UA_RelativePath *path = UA_RelativePath_new();
    //append a slash to the first position of the relative path
    char *temp = (char*) UA_calloc(strlen(yytext)+2, sizeof(char));
    char *slash = "/";
    strcpy(temp, slash);
    strcat(temp, yytext);
    UA_String parsed_string = UA_String_fromChars(temp);
    UA_StatusCode retval = UA_RelativePath_parse(path, parsed_string);
    if(retval != UA_STATUSCODE_GOOD){
        printf("failed to parse the browsepath");
    }
    UA_QualifiedName val;
    UA_QualifiedName_init(&val);
    UA_QualifiedName_copy(&path->elements[0].targetName, &val);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    free(temp);
}

static void set_up_variant_from_guid(char *yytext, UA_Variant *litvalue){
    UA_Guid val;
    UA_Guid_init(&val);
    UA_String str = UA_String_fromChars(yytext);
    UA_Guid_parse(&val, str);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_GUID]);
}

static void set_up_variant_from_int64(char *yytext, UA_Variant *litvalue){
    UA_Int64 val = (UA_Int64) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT64]);
}

static void set_up_variant_from_localized(char *yytext, UA_Variant *litvalue){
    UA_LocalizedText val = UA_LOCALIZEDTEXT("en-us", yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
}

static void set_up_variant_from_uint16(char *yytext, UA_Variant *litvalue){
    UA_UInt16 val = (UA_UInt16) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_UINT16]);
}

static void set_up_variant_from_uint32(char *yytext, UA_Variant *litvalue){
    UA_UInt32 val = (UA_UInt32) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_UINT32]);
}

static void set_up_variant_from_uint64(char *yytext, UA_Variant *litvalue){
    UA_UInt64 val = (UA_UInt64) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT64]);
}

static void set_up_variant_from_int16(char *yytext, UA_Variant *litvalue){
    UA_Int16 val = (UA_Int16) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT16]);
}

static void set_up_variant_from_int32(char *yytext, UA_Variant *litvalue){
    UA_Int32 val = (UA_Int32) atoi(yytext);
    UA_Variant_setScalarCopy(litvalue, &val, &UA_TYPES[UA_TYPES_INT32]);
}

static void create_literal_operand(char *string, UA_LiteralOperand *lit){
    UA_Variant target_value; // = UA_Variant_new();
    UA_Variant_init(&target_value);
    UA_ByteString *input_val = UA_ByteString_new();
    input_val->length = strlen(string);
    input_val->data = (UA_Byte*)string;
    UA_StatusCode ret_val = UA_decodeJson(input_val, (void *)&target_value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(ret_val != UA_STATUSCODE_GOOD){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Parsing failed with errorcode: %s", UA_StatusCode_name(ret_val));
    }
    UA_Variant_copy(&target_value, &lit->value);
    UA_Variant_clear(&target_value);
    UA_ByteString_delete(input_val);
}

static void set_up_parsed_string(char *yytext, char **temp, bool first_element, size_t *curly_bracket_ctr){
    char *first_char = "\"" ;
    if(first_element == true){
        first_char = "{\"";
        (*curly_bracket_ctr)++;
    }
    char *last_char = "\":";
    if(!*temp){
        *temp = (char*) UA_calloc(strlen(first_char)+strlen(yytext)+strlen(last_char)+1, sizeof(char));
        strcpy(*temp, first_char);
    }
    else{
        *temp = (char*) UA_realloc(*temp, (strlen(first_char)+strlen(*temp)+strlen(yytext)+strlen(last_char)+1)*sizeof(char));
        strcat(*temp, first_char);
    }
    strcat(*temp, yytext);
    strcat(*temp, last_char);
}

static void append_json_string(char **string, char *yytext, size_t *curly_bracket_ctr ){
    char *current_string = *string;
    char* open_curly_bracket = "{";
    char* close_curly_bracket = "}";
    int first_element = strcmp(yytext, open_curly_bracket);
    int last_element = strcmp(yytext, close_curly_bracket);

    char *special_char = "&,";
    char *comma = ",";

    int check = strcmp(special_char, yytext);
    if(check == 0) {
        size_t new_size = strlen(current_string) + 2;
        *string = (char *)UA_realloc(current_string, new_size);
        strcat(*string, comma);
    } else {
        size_t new_size = strlen(current_string) + 2;
        *string = (char *)UA_realloc(current_string, new_size);
        strcat(*string, yytext);
    }
    if(first_element == 0){
        (*curly_bracket_ctr)++;
    }
    if(last_element == 0){
        (*curly_bracket_ctr)--;
    }
}

static void set_up_ext_from_oftype(UA_ExtensionObject *obj, UA_Variant *var){
    UA_LiteralOperand *lit = UA_LiteralOperand_new();
    lit->value = *var;
    UA_ExtensionObject_init(obj);
    obj->encoding = UA_EXTENSIONOBJECT_DECODED;
    obj->content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    obj->content.decoded.data = lit;
    free(var);
}

static UA_Variant* create_variant_from_nodeId(UA_NodeId *id){
    UA_Variant *var = UA_Variant_new();
    UA_Variant_setScalarCopy(var, id, &UA_TYPES[UA_TYPES_NODEID]);
    UA_NodeId_clear(id);
    return var;
}

static void check_SAO(UA_SimpleAttributeOperand *sao){
    if(sao->typeDefinitionId.identifier.numeric == 0 && sao->typeDefinitionId.identifierType == UA_NODEIDTYPE_NUMERIC){
        sao->typeDefinitionId = UA_NODEID_NUMERIC(0, 2041);
    }
    //check for the default attributeId
    if(sao->attributeId == 0){
        sao->attributeId = 13;
    }
    if(sao->indexRange.length == 0){
        UA_String_init(&sao->indexRange);
        sao->indexRange =UA_String_fromChars("");
    }
}

static void set_up_index_range(char *str, UA_String *indexRange){
    *indexRange = UA_String_fromChars(str);
}

static void append_select_clauses(UA_SimpleAttributeOperand **select_clauses, size_t *sao_size, UA_SimpleAttributeOperand *sao){
    if(*sao_size == 0){
        *sao_size = (size_t) 1;
        *select_clauses = (UA_SimpleAttributeOperand*) UA_Array_new(*sao_size, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        UA_SimpleAttributeOperand_init(select_clauses[0]);
        UA_SimpleAttributeOperand_copy(sao, select_clauses[0]);
    }
    else{
        UA_StatusCode ret_val = UA_Array_append((void **)select_clauses, sao_size, sao, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        if (ret_val != UA_STATUSCODE_GOOD){
            printf("Array Increase failed\n");
        }
    }
}

static void set_up_attributeId(char *yytext, UA_UInt32 *attid){
    *attid = (UA_UInt32) atoi(yytext);
}

static void concatenate_nodeid_string(char **str, char *idx){
    *str = (char *) UA_calloc(strlen(idx)+1, sizeof(char));
    strcpy(*str, idx);
}

static void set_up_nodeId(UA_NodeId *nodeId, char* string){
    UA_String str = UA_String_fromChars(string);
    UA_NodeId_parse(nodeId, str);
    UA_String_clear(&str);
}

static void set_up_guid_identifier(char **str, char *yytext){
    char* i = "g=";
    if(!*str){
        *str = (char*) UA_calloc(strlen(i)+strlen(yytext)+1, sizeof(char));
        strcpy(*str, i);
        strcat(*str, yytext);
    }
    else{
        *str = (char*) UA_realloc(*str, (strlen(*str)+strlen(i)+strlen(yytext)+1)*sizeof(char));
        strcat(*str, i);
        strcat(*str, yytext);
    }
    //free(test);
}

static void set_up_bytestring_identifier(char **str, char *yytext){
    char *i = "b=";
    //case no index element
    if(!*str){
        *str = (char*) UA_calloc(strlen(i)+strlen(yytext)+1, sizeof(char));
        strcpy(*str, i);
        strcat(*str, yytext);
    }
    else{
        *str = (char*) UA_realloc(*str, (strlen(*str)+strlen(i)+strlen(yytext)+1)*sizeof(char));
        strcat(*str, i);
        strcat(*str, yytext);
    }
}

static void set_up_string_identifier(char **str, char *yytext){
    char *s = "s=";
    if(!*str){
        *str = (char*) UA_calloc(strlen(s)+strlen(yytext)+1, sizeof(char));
        strcpy(*str, s);
        strcat(*str, yytext);
    }
    else{
        *str = (char*) UA_realloc(*str, (strlen(*str)+strlen(s)+strlen(yytext)+1)*sizeof(char));
        strcat(*str, s);
        strcat(*str, yytext);
    }
}

static void set_up_numeric_identifier(char **str, char *yytext){
    char* i = "i=";
    if(!*str){
        *str = (char*) UA_calloc(strlen(i)+strlen(yytext)+1, sizeof(char));
        strcpy(*str, i);
        strcat(*str, yytext);
    }
    else{
        *str = (char*) UA_realloc(*str, (strlen(*str)+strlen(i)+strlen(yytext)+1)*sizeof(char));
        strcat(*str, i);
        strcat(*str, yytext);
    }
}

static void set_up_namespaceidx(char **str, char *yytext){
    *str = (char*) UA_calloc(4, sizeof(char));
    char *ns = "ns=";
    strcpy(*str, ns);
    *str = (char*) UA_realloc(*str, (strlen(*str)+strlen(yytext)+1)*sizeof(char));
    strcat(*str, yytext);
    char *semicolon = ";";
    *str = (char *) UA_realloc(*str, (strlen(*str)+strlen(semicolon)+1)*sizeof(char));
    strcat(*str, semicolon);
}

static void create_nodeId(struct local *value){
    char *string;
    concatenate_nodeid_string(&string, value->value.str);
    free(value->value.str);
    set_up_nodeId(&value->value.id, string);
    free(string);
}

static void append_sao_typeId(struct local *value){
    UA_NodeId temp;
    UA_NodeId_init(&temp);
    UA_NodeId_copy(&value->value.id, &temp);
    UA_NodeId_copy(&temp, &value->value.sao.typeDefinitionId);
    UA_NodeId_clear(&temp);
}

static void store_nodeId_as_variant(struct local *value){
    UA_NodeId temp;
    UA_NodeId_init(&temp);
    UA_NodeId_copy(&value->value.id, &temp);
    UA_NodeId_clear(&value->value.id);
    value->value.var = *(UA_Variant*) create_variant_from_nodeId(&temp);
    UA_NodeId_clear(&temp);
}

static void append_select_clauses_element(UA_SimpleAttributeOperand *local_sao, struct global_context *context){
    check_SAO(local_sao);
    UA_Select_Clauses_List *sao = (UA_Select_Clauses_List*) UA_calloc(1, sizeof(UA_Select_Clauses_List));
    UA_SimpleAttributeOperand_init(&sao->sao);
    UA_SimpleAttributeOperand_copy(local_sao, &sao->sao);
    TAILQ_INSERT_TAIL(&context->select_clauses_elements, sao, select_entries);
    for(size_t i =0 ; i< local_sao->browsePathSize; i++){
        UA_QualifiedName_clear(&local_sao->browsePath[i]);
    }
    UA_SimpleAttributeOperand_clear(local_sao);
}

static void set_up_select_clauses(struct global_context *context, size_t *select_clauses_size, UA_SimpleAttributeOperand **select_clauses){
    UA_Select_Clauses_List *sao_init, *sao_fin;
    TAILQ_FOREACH_SAFE(sao_init, &context->select_clauses_elements, select_entries, sao_fin){
        append_select_clauses(select_clauses, select_clauses_size, &sao_init->sao);
        for(size_t i =0 ; i< sao_init->sao.browsePathSize; i++){
            UA_QualifiedName_clear(&sao_init->sao.browsePath[i]);
        }
        UA_SimpleAttributeOperand_clear(&sao_init->sao);
        TAILQ_REMOVE(&context->select_clauses_elements, sao_init, select_entries);
        free(sao_init);
    };
}

static void add_new_filter_element(size_t *nbr_filter_elements, UA_Where_Clause_List **children ){
    if((*nbr_filter_elements) ==0 ){
        *children = (UA_Where_Clause_List*) UA_calloc(1, sizeof(UA_Where_Clause_List));
        (*nbr_filter_elements)++;
    }
    else{
        (*nbr_filter_elements)++;
        *children = (UA_Where_Clause_List*) UA_realloc((*children), (*nbr_filter_elements) * sizeof(UA_Where_Clause_List));
        memset(&(*children)[*nbr_filter_elements-1], 0, sizeof(UA_Where_Clause_List));
    }
    (*children)[(*nbr_filter_elements)-1].identifier = FILTERELEMENT;
}

static void add_new_branch_element(size_t *nbr_filter_elements, UA_Where_Clause_List **children ){
    if( (*nbr_filter_elements) ==0 ){
        (*children) = (UA_Where_Clause_List*) UA_calloc(1, sizeof(UA_Where_Clause_List));
        (*nbr_filter_elements)++;
    }
    else{
        (*nbr_filter_elements)++;
        (*children) = (UA_Where_Clause_List*) UA_realloc(*children, (*nbr_filter_elements) * sizeof(UA_Where_Clause_List));
    }
    (*children)[(*nbr_filter_elements)-1].identifier = BRANCHELEMENT;
}

static void add_linked_list_filter_item(size_t *filter_elements, UA_Where_Clause_List **children){
    if((*children)[*filter_elements-1].identifier == FILTERELEMENT){
        printf("filterelement\n");
    }
    if((*children)[*filter_elements-1].element.filt.element.nbr_operands == 0){
        printf("case new item\n");
        (*children)[*filter_elements-1].element.filt.element.operand_list = (UA_OperandReference*) UA_calloc(1, sizeof(UA_OperandReference));
        (*children)[*filter_elements-1].element.filt.element.nbr_operands=1;
    }
    else{
        printf("case append list\n");
        (*children)[*filter_elements-1].element.filt.element.nbr_operands++;
        printf("the number of operands are %zu \n", (*children)[*filter_elements-1].element.filt.element.nbr_operands);
        (*children)[*filter_elements-1].element.filt.element.operand_list = (UA_OperandReference*) UA_realloc((*children)[*filter_elements-1].element.filt.element.operand_list, (*children)[*filter_elements-1].element.filt.element.nbr_operands*sizeof(UA_OperandReference));
    }
}

static void set_reference(char **reference, char *input){
    *reference = (char*) UA_calloc(strlen(input)+1, sizeof(char));
    strcpy(*reference, input);
}

static void check_empty_list(size_t *filter_elements, UA_Where_Clause_List **children){
    printf("check empty list\n");
    if(*filter_elements ==0) {
        printf("add list element\n");
        (*children) = (UA_Where_Clause_List *)UA_calloc(1, sizeof(UA_Where_Clause_List));
        (*filter_elements)++;
    }
}

static void set_up_branch_element_identifier(size_t *branch_nbr, char **ref){
    char *ref_identifier = "branch_reference_";
    char ref_nbr[128];
    sprintf(ref_nbr, "%zu", *branch_nbr);
    *ref = (char*) UA_calloc(strlen(ref_identifier)+1+strlen(ref_nbr), sizeof(char));
    strcpy(*ref, ref_identifier);
    strcat(*ref, ref_nbr);
    (*branch_nbr)++;
}

static void handle_branch_operator(size_t *filter_elements, UA_Where_Clause_List **children, UA_FilterOperator filt_operator, size_t *branch_ctr){
    add_new_branch_element(filter_elements, children);
    (*children)[(*filter_elements)-1].identifier = BRANCHELEMENT;
    memset(&(*children)[(*filter_elements)-1].element.branch, 0, sizeof(UA_branch_element));
    (*children)[(*filter_elements)-1].element.branch.child_nbr = 0;
    (*children)[(*filter_elements)-1].element.branch.oper = filt_operator;
    set_up_branch_element_identifier(branch_ctr, &(*children)[*filter_elements-1].element.branch.ref);
}

static void remove_list_element(size_t *filter_elements, UA_Where_Clause_List **children){
    if((*children)[*filter_elements-1].identifier ==BRANCHELEMENT){
        //clear the memory of the child ref entries
        for(size_t i = 0; i< (*children)[*filter_elements-1].element.branch.child_nbr; i++){
            memset((*children)[*filter_elements-1].element.branch.child_ref[i],0, sizeof(char));
            free((*children)[*filter_elements-1].element.branch.child_ref[i]);
        }
        //clear the memory of the child ref array
        memset((*children)[*filter_elements-1].element.branch.child_ref, 0, sizeof(char*));
        free((*children)[*filter_elements-1].element.branch.child_ref);
        //clear the momry of the element ref
        memset((*children)[*filter_elements-1].element.branch.ref, 0, sizeof(char));
        free((*children)[*filter_elements-1].element.branch.ref);
    }
    else{
        free((*children)[*filter_elements-1].element.filt.ref);
        for(size_t i=0; i< (*children)[*filter_elements-1].element.filt.element.nbr_operands; i++){
            if((*children)[*filter_elements-1].element.filt.element.operand_list[i].identifier == ELEMENTOPERAND){
                free((*children)[*filter_elements-1].element.filt.element.operand_list[i].operand.element_identifier);
            }
        }
        free((*children)[*filter_elements-1].element.filt.element.operand_list);
    }
    //resize the list
    if(*filter_elements > 1){
        (*children) = (UA_Where_Clause_List *) UA_realloc((*children), (*filter_elements - 1) * sizeof(UA_Where_Clause_List));
        (*filter_elements)--;
    }
    else{
        free(*children);
        *filter_elements = 0;
    }
}

static void add_linked_list_element(size_t *filter_elements, UA_Where_Clause_List **children, struct where_list *linked_list){
    //copy the where element structure and add it to the linked lists
    struct where_elements *where = (struct where_elements*) UA_calloc(1, sizeof(struct where_elements));
    where->identifier = (*children)[*filter_elements-1].identifier;
    if((*children)[*filter_elements-1].identifier == FILTERELEMENT){
        set_reference(&where->element.filt.ref, (*children)[*filter_elements-1].element.filt.ref);
        where->element.filt.element.filter = (*children)[*filter_elements-1].element.filt.element.filter;
        where->element.filt.element.nbr_operands = (*children)[*filter_elements-1].element.filt.element.nbr_operands;
        where->element.filt.element.operand_list = (UA_OperandReference*) UA_calloc(where->element.filt.element.nbr_operands, sizeof(UA_OperandReference));
        for(size_t i=0; i < where->element.filt.element.nbr_operands; i++){
            where->element.filt.element.operand_list[i].identifier = (*children)[*filter_elements-1].element.filt.element.operand_list[i].identifier;
            if((*children)[*filter_elements-1].element.filt.element.operand_list[i].identifier == ELSE){
                UA_ExtensionObject_copy(&(*children)[*filter_elements-1].element.filt.element.operand_list[i].operand.extobj, &where->element.filt.element.operand_list[i].operand.extobj);
            }
            else{
                where->element.filt.element.operand_list[i].operand.element_identifier = (char*) UA_calloc(strlen((*children)[*filter_elements-1].element.filt.element.operand_list[i].operand.element_identifier)+1, sizeof(char));
                strcpy(where->element.filt.element.operand_list[i].operand.element_identifier, (*children)[*filter_elements-1].element.filt.element.operand_list[i].operand.element_identifier);
            }
        }
    }
    else{
        where->element.branch.child_nbr = (*children)[*filter_elements - 1].element.branch.child_nbr;
        where->element.branch.ref = (char *)UA_calloc(strlen((*children)[*filter_elements - 1].element.branch.ref) + 1, sizeof(char));
        strcpy(where->element.branch.ref, (*children)[*filter_elements-1].element.branch.ref);
        where->element.branch.child_ref = (char **)UA_calloc((*children)[*filter_elements - 1].element.branch.child_nbr, sizeof(char*));
        for(size_t i = 0; i< (*children)[*filter_elements-1].element.branch.child_nbr; i++){
            where->element.branch.child_ref[i] = (char*)UA_calloc(strlen((*children)[*filter_elements-1].element.branch.child_ref[i])+1, sizeof(char));
            strcpy(where->element.branch.child_ref[i], (*children)[*filter_elements-1].element.branch.child_ref[i]);
        }
    }
    TAILQ_INSERT_TAIL(&linked_list->where_clause_elements, where, where_entries);
}

static void handle_elementoperand(size_t *filter_elements, UA_Where_Clause_List **children, char *yytext){
    //check if the last entry is a branch element, if so create a new list element
    if((*children)[*filter_elements-1].identifier == BRANCHELEMENT){
        add_new_filter_element(filter_elements, children);
    }
    //linked list
    add_linked_list_filter_item(filter_elements, children);
    UA_FilterElementContainer *ptr = &(*children)[*filter_elements-1].element.filt.element;
    ptr->operand_list[ptr->nbr_operands-1].identifier = ELEMENTOPERAND;
    ptr->operand_list[ptr->nbr_operands-1].operand.element_identifier = (char*) UA_calloc(strlen(yytext)+1, sizeof(char));
    strcpy(ptr->operand_list[ptr->nbr_operands-1].operand.element_identifier, yytext);
}

static void handle_literaloperand(size_t *filter_elements, UA_Where_Clause_List **children, UA_Local_Context *local){
    check_empty_list(filter_elements, children);
    //check if the last entry is a branch element, if so create a new list element
    if((*children)[*filter_elements-1].identifier == BRANCHELEMENT){
        add_new_filter_element(filter_elements, children);
    }
    UA_LiteralOperand lit;
    UA_LiteralOperand_init(&lit);
    add_linked_list_filter_item(filter_elements, children);
    UA_FilterElementContainer *ptr = &(*children)[*filter_elements-1].element.filt.element;
    if(local->identifier == DATATYPE_STRING){
        create_literal_operand(local->value.str, &lit);
        ptr->operand_list[ptr->nbr_operands-1].identifier = ELSE;
        set_up_ext_object_from_literal(&ptr->operand_list[ptr->nbr_operands-1].operand.extobj, &lit);
    }
    else{
        UA_Variant_copy(&local->value.var, &lit.value);
        UA_Variant_clear(&local->value.var);
        ptr->operand_list[ptr->nbr_operands-1].identifier = ELSE;
        set_up_ext_object_from_literal(&ptr->operand_list[ptr->nbr_operands-1].operand.extobj, &lit);
    }
    local->identifier = DEFAULT;
    UA_LiteralOperand_clear(&lit);
}

static void handle_sao(size_t *filter_elements, UA_Where_Clause_List **children, UA_Local_Context *local){

    check_SAO(&local->value.sao);
    if(*filter_elements == 0){
        printf("empty list\n");
        add_new_filter_element(filter_elements, children);
    }
    else{
        //check if the last entry is a branch element, if so create a new list element
        if((*children)[*filter_elements-1].identifier == BRANCHELEMENT){
            printf("sao as child\n");
            add_new_filter_element(filter_elements, children);
        }
    }
    //linked list
    add_linked_list_filter_item(filter_elements, children);
    UA_FilterElementContainer *ptr = &(*children)[*filter_elements-1].element.filt.element;
    ptr->operand_list[ptr->nbr_operands-1].identifier = ELSE;
    set_up_ext_object_from_sao(&ptr->operand_list[ptr->nbr_operands-1].operand.extobj, &local->value.sao);
    //clear the memory
    for(size_t i =0 ; i< local->value.sao.browsePathSize; i++){
        UA_QualifiedName_clear(&local->value.sao.browsePath[i]);
    }
    UA_SimpleAttributeOperand_clear(&local->value.sao);
}

static void handle_oftype(size_t *filter_elements, UA_Where_Clause_List **children, UA_Local_Context *local){

    //check if the last entry is a branch element, if so create a new list element
    printf("found a odtype operatore \n");
    if((*children)[*filter_elements-1].identifier == BRANCHELEMENT){
        add_new_filter_element(filter_elements, children);
    }
    UA_Variant *var = UA_Variant_new();
    var = create_variant_from_nodeId(&local->value.id);
    //add linked list item
    add_linked_list_filter_item(filter_elements,children);

    (*children)[*filter_elements-1].element.filt.element.filter = UA_FILTEROPERATOR_OFTYPE;
    (*children)[*filter_elements-1].element.filt.element.operand_list[(*children)[*filter_elements-1].element.filt.element.nbr_operands-1].identifier = ELSE;
    set_up_ext_from_oftype(&(*children)[*filter_elements-1].element.filt.element.operand_list[(*children)[*filter_elements-1].element.filt.element.nbr_operands-1].operand.extobj, var);

}

static void add_branch_child_ref(size_t *filter_elements, UA_Where_Clause_List **children, char *ref){
    UA_branch_element *ptr = &(*children)[*filter_elements-1].element.branch;
    //allocate memeory for the string array
    if(ptr->child_nbr ==0){
        ptr->child_ref = (char **)UA_calloc(1, sizeof(char *));
        ptr->child_nbr++;
    }
    else{
        ptr->child_ref = (char **)UA_realloc(ptr->child_ref,(ptr->child_nbr + 1) *sizeof(char *));
        ptr->child_nbr++;
    }
    (*children)[*filter_elements-1].identifier = BRANCHELEMENT;
    set_reference(&ptr->child_ref[ptr->child_nbr-1], ref);
}

#endif //OPEN62541_NODEID_SETUP_FUNCTIONS_H
