/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
* When using events, the client is most often only interested in a small selection of events.
* To avoid unnecessary event transmission, the client can configure filters. These filters
* are part of the event-subscription configuration.
*
* The following example shows the general use and configuration of event-filters. This
* example is intended to be used together with the "server_random_events" example. The
* server example can emit events, which mach the below defined event-filters.
*
* */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>
#include "common.h"

#include <signal.h>
#include <stdio.h>

#define USE_FILTER_OR_TYPEOF

static UA_Boolean running = true;
#define SELECT_CLAUSE_FIELD_COUNT 3


static UA_SimpleAttributeOperand *
setupSelectClauses(size_t selectedFieldsSize, UA_QualifiedName *qName) {
   UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
       UA_Array_new(selectedFieldsSize, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
   if(!selectClauses)
       return NULL;
   for(size_t i =0; i<selectedFieldsSize; ++i) {
       UA_SimpleAttributeOperand_init(&selectClauses[i]);
   }

   for (size_t i = 0; i < selectedFieldsSize; ++i) {
       selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
       selectClauses[i].browsePathSize = 1;
       selectClauses[i].browsePath = (UA_QualifiedName*)
           UA_Array_new(selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
       if(!selectClauses[i].browsePath) {
           UA_SimpleAttributeOperand_delete(selectClauses);
           return NULL;
       }
       selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
       char fieldName[64];
       memcpy(fieldName, qName[i].name.data, qName[i].name.length);
       fieldName[qName[i].name.length] = '\0';
       selectClauses[i].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, fieldName);
   }
   return selectClauses;
}

static UA_StatusCode
setupOperandArrays(UA_ContentFilter *contentFilter){
   for(size_t i =0; i< contentFilter->elementsSize; ++i) {  /* Set Operands Arrays */
       contentFilter->elements[i].filterOperands = (UA_ExtensionObject*)
           UA_Array_new(
               contentFilter->elements[i].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
       if (!contentFilter->elements[i].filterOperands){
           UA_ContentFilter_clear(contentFilter);
           return UA_STATUSCODE_BADOUTOFMEMORY;
       }
       for(size_t n =0; n< contentFilter->elements[i].filterOperandsSize; ++n) {
           UA_ExtensionObject_init(&contentFilter->elements[i].filterOperands[n]);
       }
   }
   return UA_STATUSCODE_GOOD;
}

static void
setupTwoOperandsFilter(UA_ContentFilterElement *element, UA_UInt32 firstOperandIndex, UA_UInt32 secondOperandIndex){
   element->filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
   element->filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
   element->filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
   element->filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
   UA_ElementOperand *firstElementOperand = UA_ElementOperand_new();
   UA_ElementOperand_init(firstElementOperand);
   firstElementOperand->index = firstOperandIndex;
   UA_ElementOperand *secondElementOperand = UA_ElementOperand_new();
   UA_ElementOperand_init(secondElementOperand);
   secondElementOperand->index = secondOperandIndex;
   element->filterOperands[0].content.decoded.data = firstElementOperand;
   element->filterOperands[1].content.decoded.data = secondElementOperand;
}

static void
setupOfTypeFilter(UA_ContentFilterElement *element, UA_UInt16 nsIndex, UA_UInt32 typeId){
   element->filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
   element->filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
   UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
   UA_LiteralOperand_init(literalOperand);
   UA_NodeId *nodeId = UA_NodeId_new();
   UA_NodeId_init(nodeId);
   nodeId->namespaceIndex = nsIndex;
   nodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
   nodeId->identifier.numeric = typeId;
   UA_Variant_setScalar(&literalOperand->value, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
   element->filterOperands[0].content.decoded.data = literalOperand;
}

static void
setupLiteralOperand(UA_ContentFilterElement *element, size_t operandIndex, UA_Variant literal){
   element->filterOperands[operandIndex].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
   element->filterOperands[operandIndex].encoding = UA_EXTENSIONOBJECT_DECODED;
   UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
   UA_LiteralOperand_init(literalOperand);
   literalOperand->value = literal;
   element->filterOperands[operandIndex].content.decoded.data = literalOperand;
}

static void
setupSimpleAttributeOperand(UA_ContentFilterElement *element, size_t operandIndex, UA_SimpleAttributeOperand attributeOperand){
   element->filterOperands[operandIndex].content.decoded.type = &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND];
   element->filterOperands[operandIndex].encoding = UA_EXTENSIONOBJECT_DECODED;
   UA_SimpleAttributeOperand *simpleAttributeOperand = UA_SimpleAttributeOperand_new();
   *simpleAttributeOperand = attributeOperand;
   element->filterOperands[operandIndex].content.decoded.data = simpleAttributeOperand;
}

static UA_StatusCode
setupWhereClauses(UA_ContentFilter *contentFilter, UA_UInt16 whereClauseSize, UA_UInt16 filterSelection){
   UA_ContentFilter_init(contentFilter);
   contentFilter->elementsSize = whereClauseSize;
   contentFilter->elements  = (UA_ContentFilterElement *)
       UA_Array_new(contentFilter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
   if(!contentFilter->elements)
       return UA_STATUSCODE_BADOUTOFMEMORY;
   for(size_t i =0; i < contentFilter->elementsSize; ++i) {
       UA_ContentFilterElement_init(&contentFilter->elements[i]);
   }
   UA_StatusCode result = UA_STATUSCODE_GOOD;
   switch(filterSelection) {
       case 0: {
           contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[2].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[0].filterOperandsSize = 2;
           contentFilter->elements[1].filterOperandsSize = 1;
           contentFilter->elements[2].filterOperandsSize = 1;
           /* Setup Operand Arrays */
           result = setupOperandArrays(contentFilter);
           if(result != UA_STATUSCODE_GOOD) {
               UA_ContentFilter_clear(contentFilter);
               return UA_STATUSCODE_BADCONFIGURATIONERROR;
           }
           /* first Element (OR) */
           setupTwoOperandsFilter(&contentFilter->elements[0], 1, 2);
           /* second Element (OfType) */
           setupOfTypeFilter(&contentFilter->elements[1], 1, 5003);
           /* third Element (OfType) */
           setupOfTypeFilter(&contentFilter->elements[2], 0,
                             UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE);
           break;
       }
       case 1: {
           contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[0].filterOperandsSize = 1;
           /* Setup Operand Arrays */
           result = setupOperandArrays(contentFilter);
           if(result != UA_STATUSCODE_GOOD) {
               UA_ContentFilter_clear(contentFilter);
               return UA_STATUSCODE_BADCONFIGURATIONERROR;
           }
           setupOfTypeFilter(&contentFilter->elements[0], 1, 5001);
           break;
       }
       case 2: {
           contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[1].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[2].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[3].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[4].filterOperator = UA_FILTEROPERATOR_OR;
           contentFilter->elements[5].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[6].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[7].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[8].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[9].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[10].filterOperator = UA_FILTEROPERATOR_OFTYPE;

           contentFilter->elements[0].filterOperandsSize = 2;
           contentFilter->elements[1].filterOperandsSize = 2;
           contentFilter->elements[2].filterOperandsSize = 2;
           contentFilter->elements[3].filterOperandsSize = 2;
           contentFilter->elements[4].filterOperandsSize = 2;
           contentFilter->elements[5].filterOperandsSize = 1;
           contentFilter->elements[6].filterOperandsSize = 1;
           contentFilter->elements[7].filterOperandsSize = 1;
           contentFilter->elements[8].filterOperandsSize = 1;
           contentFilter->elements[9].filterOperandsSize = 1;
           contentFilter->elements[10].filterOperandsSize = 1;

           /* Setup Operand Arrays */
           result = setupOperandArrays(contentFilter);
           if(result != UA_STATUSCODE_GOOD) {
               UA_ContentFilter_clear(contentFilter);
               return UA_STATUSCODE_BADCONFIGURATIONERROR;
           }

           // init or clauses
           setupTwoOperandsFilter(&contentFilter->elements[0], 1, 2);
           setupTwoOperandsFilter(&contentFilter->elements[1], 3, 4);
           setupTwoOperandsFilter(&contentFilter->elements[2], 5, 6);
           setupTwoOperandsFilter(&contentFilter->elements[3], 7, 8);
           setupTwoOperandsFilter(&contentFilter->elements[4], 9, 10);
           // init oftype
           setupOfTypeFilter(&contentFilter->elements[5], 1, 5000);
           setupOfTypeFilter(&contentFilter->elements[6], 1, 5001);
           setupOfTypeFilter(&contentFilter->elements[7], 1, 5002);
           setupOfTypeFilter(&contentFilter->elements[8], 1, 5003);
           setupOfTypeFilter(&contentFilter->elements[9], 1, 5004);
           setupOfTypeFilter(&contentFilter->elements[10], 0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE);
           break;
       }
       case 3: {
           contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_AND;
           contentFilter->elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[2].filterOperator = UA_FILTEROPERATOR_AND;
           contentFilter->elements[3].filterOperator = UA_FILTEROPERATOR_EQUALS;
           contentFilter->elements[4].filterOperator = UA_FILTEROPERATOR_GREATERTHAN;

           contentFilter->elements[0].filterOperandsSize = 2;
           contentFilter->elements[1].filterOperandsSize = 1;
           contentFilter->elements[2].filterOperandsSize = 2;
           contentFilter->elements[3].filterOperandsSize = 2;
           contentFilter->elements[4].filterOperandsSize = 2;

           /* Setup Operand Arrays */
           result = setupOperandArrays(contentFilter);
           if(result != UA_STATUSCODE_GOOD) {
               UA_ContentFilter_clear(contentFilter);
               return UA_STATUSCODE_BADCONFIGURATIONERROR;
           }

           // init clauses
           setupTwoOperandsFilter(&contentFilter->elements[0], 1, 2);
           setupTwoOperandsFilter(&contentFilter->elements[2], 3, 4);
           setupOfTypeFilter(&contentFilter->elements[1], 1, 5000);

           UA_Variant literalContent;
           UA_Variant_init(&literalContent);
           UA_UInt32 literal_value = 99;
           UA_Variant_setScalarCopy(&literalContent, &literal_value, &UA_TYPES[UA_TYPES_UINT32]);

           setupLiteralOperand(&contentFilter->elements[3], 0, literalContent);
           setupLiteralOperand(&contentFilter->elements[3], 1, literalContent);

           UA_SimpleAttributeOperand sao;
           UA_SimpleAttributeOperand_init(&sao);
           sao.attributeId = UA_ATTRIBUTEID_VALUE;
           sao.typeDefinitionId = UA_NODEID_NUMERIC(0, 5000);
           sao.browsePathSize = 1;
           UA_QualifiedName *qn = UA_QualifiedName_new();
           *qn = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
           sao.browsePath = qn;
           setupSimpleAttributeOperand(&contentFilter->elements[4], 0, sao);
           setupLiteralOperand(&contentFilter->elements[4], 1, literalContent);
           break;
       }
       case 4: {
           contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_AND;
           contentFilter->elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE;
           contentFilter->elements[2].filterOperator = UA_FILTEROPERATOR_GREATERTHAN;

           contentFilter->elements[0].filterOperandsSize = 2;
           contentFilter->elements[1].filterOperandsSize = 1;
           contentFilter->elements[2].filterOperandsSize = 2;

           /* Setup Operand Arrays */
           result = setupOperandArrays(contentFilter);
           if(result != UA_STATUSCODE_GOOD) {
               UA_ContentFilter_clear(contentFilter);
               return UA_STATUSCODE_BADCONFIGURATIONERROR;
           }

           // init clauses
           setupTwoOperandsFilter(&contentFilter->elements[0], 1, 2);
           setupOfTypeFilter(&contentFilter->elements[1], 1, 5000);

           UA_Variant literalContent;
           UA_Variant_init(&literalContent);
           UA_UInt32 literal_value = 99;
           UA_Variant_setScalarCopy(&literalContent, &literal_value, &UA_TYPES[UA_TYPES_UINT32]);

           UA_SimpleAttributeOperand sao;
           UA_SimpleAttributeOperand_init(&sao);
           sao.attributeId = UA_ATTRIBUTEID_VALUE;
           sao.typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
           sao.browsePathSize = 1;
           UA_QualifiedName *qn = UA_QualifiedName_new();
           *qn = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
           sao.browsePath = qn;
           setupSimpleAttributeOperand(&contentFilter->elements[2], 0, sao);
           setupLiteralOperand(&contentFilter->elements[2], 1, literalContent);
           break;
       }
       default:
           UA_ContentFilter_clear(contentFilter);
           return UA_STATUSCODE_BADCONFIGURATIONERROR;
   }
   return UA_STATUSCODE_GOOD;
}


static void compare_sao(UA_SimpleAttributeOperand *src, UA_SimpleAttributeOperand *dest, bool *equal){
    if(!UA_NodeId_equal(&src->typeDefinitionId, &dest->typeDefinitionId)){
        *equal = false;
    }
    if(!UA_String_equal(&src->indexRange, &dest->indexRange)){
        printf("SAO have different index Ranges\n");
        *equal = false;
    }
    if(src->attributeId != dest->attributeId){
        printf("SAO have different attribute ids. srs has %d, dest has %d\n", src->attributeId, dest->attributeId);
        *equal = false;
    }
    if(src->browsePathSize != dest->browsePathSize){
        printf("SAO have different browsepath sizes ids. srs has %zu, dest has %zu \n", src->browsePathSize, dest->browsePathSize);
        *equal = false;
    }
    else{
        for(size_t j=0; j<src->browsePathSize; j++){
            if(!UA_QualifiedName_equal(&src->browsePath[j], &dest->browsePath[j])){
                printf("SAO have different browsepathelements on pos %zu \n", j);
                *equal = false;
            }
        }
    }
}

static void UA_EventFilter_equal(UA_EventFilter *src, UA_EventFilter *dest){
    bool equal = true;
    if(src->selectClausesSize != dest->selectClausesSize){
        printf("different select clause size. srs has %zu and dest has %zu\n", src->selectClausesSize, dest->selectClausesSize);
        equal = false;
    }
    else{
        for(size_t i=0; i<src->selectClausesSize; i++){
            printf("check select_clauses position %zu\n", i);
            compare_sao(&src->selectClauses[i], &dest->selectClauses[i], &equal);
        }
    }
    if(src->whereClause.elementsSize != dest->whereClause.elementsSize){
        printf("different where clause size. srs has %zu and dest has %zu\n", src->whereClause.elementsSize, dest->whereClause.elementsSize);
        equal = false;
    }
    else{
        for(size_t i=0; i< src->whereClause.elementsSize; i++){
            printf("check contentFilter position %zu\n", i);
            if(src->whereClause.elements[i].filterOperator != dest->whereClause.elements[i].filterOperator){
                printf("different FilterOperators are used. src has %d, dest has %d\n", src->whereClause.elements[i].filterOperator, dest->whereClause.elements[i].filterOperator);
                equal = false;
            }
            if(src->whereClause.elements[i].filterOperandsSize != dest->whereClause.elements[i].filterOperandsSize){
                printf("different number of operands. srs has %zu and dest has %zu\n", src->whereClause.elements[i].filterOperandsSize, dest->whereClause.elements[i].filterOperandsSize);
                equal = false;
            }
            else{
                for(size_t j=0; j< src->whereClause.elements[i].filterOperandsSize; j++){
                    if(!UA_NodeId_equal(&src->whereClause.elements[i].filterOperands[j].content.decoded.type->typeId, &dest->whereClause.elements[i].filterOperands[j].content.decoded.type->typeId)){
                        printf("operands have different typeids.\n");
                        equal = false;
                    }
                    else{
                        UA_NodeId sao_id = UA_NODEID_NUMERIC(0, UA_NS0ID_SIMPLEATTRIBUTEOPERAND);
                        UA_NodeId elem_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ELEMENTOPERAND);
                        UA_NodeId lit_id = UA_NODEID_NUMERIC(0, UA_NS0ID_LITERALOPERAND);
                        if(UA_NodeId_equal(&sao_id, &src->whereClause.elements[i].filterOperands[j].content.decoded.type->typeId)){
                            UA_SimpleAttributeOperand *sao_1 = (UA_SimpleAttributeOperand*) src->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            UA_SimpleAttributeOperand *sao_2 = (UA_SimpleAttributeOperand*) dest->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            compare_sao(sao_1, sao_2, &equal);
                        }
                        if(UA_NodeId_equal(&elem_id, &src->whereClause.elements[i].filterOperands[j].content.decoded.type->typeId)){
                            UA_ElementOperand *val_1 = (UA_ElementOperand*) src->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            UA_ElementOperand *val_2 = (UA_ElementOperand*) dest->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            UA_String out_1 = UA_STRING_NULL;
                            UA_print(val_1, &UA_TYPES[UA_TYPES_ELEMENTOPERAND], &out_1);
                            UA_String_clear(&out_1);
                            UA_String out_2 = UA_STRING_NULL;
                            UA_print(val_2, &UA_TYPES[UA_TYPES_ELEMENTOPERAND], &out_2);
                            if(UA_String_equal(&out_1, &out_2) !=0){
                            //if(val_1->index != val_2->index){
                                printf("different elements referenced\n");
                                equal = false;
                            }
                        }
                        if(UA_NodeId_equal(&lit_id, &src->whereClause.elements[i].filterOperands[j].content.decoded.type->typeId)){
                            UA_LiteralOperand *val_1 = (UA_LiteralOperand*) src->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            UA_LiteralOperand *val_2 = (UA_LiteralOperand*) dest->whereClause.elements[i].filterOperands[j].content.decoded.data;
                            UA_String out_1 = UA_STRING_NULL;
                            UA_print(val_1, &UA_TYPES[UA_TYPES_LITERALOPERAND], &out_1);
                            UA_String_clear(&out_1);
                            UA_String out_2 = UA_STRING_NULL;
                            UA_print(val_2, &UA_TYPES[UA_TYPES_LITERALOPERAND], &out_2);
                            if(UA_String_equal(&out_1, &out_2) !=0){
                                printf("different literals referenced\n");
                                equal = false;
                            }
                            UA_String_clear(&out_1);
                            UA_String_clear(&out_2);
                        }
                    }
                }
            }
        }
    }
    if(!equal){
        printf("matching of the event filter failed. See previous output\n");
    }
    else{
        printf("filter matching succeeded\n");
    }
}
static void clear_event_filter(UA_EventFilter *filter){
    for(size_t i=0; i< filter->selectClausesSize; i++){
        for(size_t j=0; j< filter->selectClauses[i].browsePathSize; j++){
            UA_QualifiedName_clear(&filter->selectClauses[i].browsePath[j]);
        }
        UA_NodeId_clear(&filter->selectClauses[i].typeDefinitionId);
        UA_String_clear(&filter->selectClauses[i].indexRange);
        UA_SimpleAttributeOperand_clear(&filter->selectClauses[i]);
    }
    for(size_t i=0; i<filter->whereClause.elementsSize; i++){
        for(size_t j=0; j< filter->whereClause.elements[i].filterOperandsSize; j++){
            UA_ExtensionObject_clear(&filter->whereClause.elements[i].filterOperands[j]);
        }
        UA_ContentFilterElement_clear(&filter->whereClause.elements[i]);
    }
    UA_ContentFilter_clear(&filter->whereClause);
    UA_EventFilter_clear(filter);
}

static void check_eventfilter(UA_EventFilter *filter){
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    if(memcmp(&empty_filter, filter, sizeof(UA_EventFilter)) == 0){
        printf("failed to parse the filter\n");
    }
    else{
        printf("parsing succeeded\n");
        UA_String out = UA_STRING_NULL;
        UA_print(filter, &UA_TYPES[UA_TYPES_EVENTFILTER], &out);
        printf("%.*s\n", (int)out.length, out.data);
        UA_String_clear(&out);
    }
}
#define number_examples 5
char test_queries[number_examples][128][128] = {
    {"case_0.txt", "case_0_0.txt", "case_0_1.txt"},
    {"case_1.txt", "case_1_0.txt"},
    {"case_2.txt", "case_2_0.txt", "case_2_1.txt", "case_2_2.txt", "case_2_3.txt"},
    {"case_3.txt"},
    {"case_4.txt"}
};
size_t matrix_structure[number_examples] = {3, 2, 5, 1, 1};
size_t where_clause_size[number_examples] = {3, 1, 11, 5, 3};



static void run_tests(size_t nbr_examples){
    char *path_to_example_folder = "../../../examples/eventfilter_parser/tests/test_queries/";
    for (size_t i=0; i < nbr_examples; i++){
        UA_EventFilter src;
        UA_EventFilter_init(&src);
        UA_QualifiedName qm[SELECT_CLAUSE_FIELD_COUNT];
        qm[0].namespaceIndex = 0;
        qm[0].name = UA_STRING("Message");
        qm[1].namespaceIndex = 0;
        qm[1].name = UA_STRING("Severity");
        qm[2].namespaceIndex = 0;
        qm[2].name = UA_STRING("EventType");
        src.selectClauses = setupSelectClauses(SELECT_CLAUSE_FIELD_COUNT, qm);
        src.selectClausesSize = SELECT_CLAUSE_FIELD_COUNT;
        setupWhereClauses(&src.whereClause, where_clause_size[i], i);

        for(size_t j=0; j < matrix_structure[i]; j++){
            char temp[512];
            strcpy(temp, path_to_example_folder);
            strcat(temp, test_queries[i][j]);
            UA_EventFilter target;
            UA_EventFilter_init(&target);
            printf("check example: %s\n", temp);
            UA_ByteString content = loadFile(temp);

            UA_EventFilter_parse(&content, &target);
            printf("parsed filter\n");
            check_eventfilter(&target);
            printf("implemented filter\n");
            check_eventfilter(&src);
            UA_EventFilter_equal(&src, &target);
            /*if(memcmp(&src, &target, sizeof(UA_EventFilter)) == 0 ){
                printf("filter matching succeeded \n");
            }
            else{
                printf("filter query is not the same as the implemented\n");
            }*/
            clear_event_filter(&target);
        }
        //UA_Array_delete(&src.selectClauses, src.selectClausesSize, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        //UA_Array_delete(&src.whereClause.elements, src.whereClause.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        //UA_EventFilter_clear(&src);
        //clear_event_filter(&src);
    }
}

static void stopHandler(int sig) {
   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
   running = false;
}

int main(int argc, char *argv[]) {
   signal(SIGINT, stopHandler);
   signal(SIGTERM, stopHandler);


   UA_Client *client = UA_Client_new();
   UA_ClientConfig_setDefault(UA_Client_getConfig(client));

   UA_StatusCode retval = UA_Client_connect(client, argv[1]);
   if(retval != UA_STATUSCODE_GOOD) {
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Could not connect");
       UA_Client_delete(client);
       return 0;
   }

   run_tests(number_examples);


   while(running)
       UA_Client_run_iterate(client, 100);

   UA_Client_disconnect(client);
   UA_Client_delete(client);
   return 0;
}
