//
// Created by flo47663 on 21.09.2022.
//
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

#include <signal.h>

#define USE_FILTER_OR_TYPEOF

static UA_Boolean running = true;
#define SELECT_CLAUSE_FIELD_COUNT 3

typedef struct {
    UA_UInt16 nsIndex;
    UA_UInt32 typeId;
} UA_of_Type_Values;

typedef struct {
    UA_ExtensionObject operand_one;
    UA_ExtensionObject operand_two;
} Literal_Operand_Comparison;

/**
 * Setting up SelectClauses
 * ^^^^^^^^^^^^^^^^^^^
 * The SelectClauses specifies the EventFields of an Event.
 * It is represented in a SimpleAttributeOperand Array.
 * Each SimpleAttributeOperand represents one EventField that should be returned.
 * In this Example we are selecting two SimpleAttributeOperands to be returned, one as a Message and one as Severity.
 */
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
/******************
 * its also possible to compare non integer values
 ***********************/
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
/*************************
 * change so that the browsename + browsepath is given and the NodeId is browsed
 ************************/
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
/******************
 * the literal is axtracted from the statement -> transform to variant
 ********************/
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






/**
 * Setting up WhereClauses
 * ^^^^^^^^^^^^^^^^^^^
 * The WhereClause defines the filtering criteria of an EventFilter, represented as an
 * ContentFilter structure. The ContentFilter structure contains "ContentFilterElements",
 * which contain a operator and a array of operands. The ContentFilter has a Tree-structure.
 * The example allows multiple content-filters (where clauses) which can be selected
 * with the filterSelection paramete.
 *
 * filterSelection 0:
 *      ( (OfType AUDITEVENTTYPE ) (or) (OfType EVENTQUEUEOVERFLOWEVENTTYPE) )
 * filterSelection 2:
 *      ((EventTypeId == NodeID("StartScanEvent")           ||
 *        (EventTypeId == NodeID("DcpScanFinishedEvent")    ||
 *        (EventTypeId == NodeID("ScanFinishedEvent")       ||
 *        (EventTypeId == NodeID("CancelScanEvent")         ||
 *        (EventTypeId == NodeID("CancelScanFinishedEvent") ||
 *        (EventTypeId == NodeID("ShutdownEvent"))
 * filterSelection 3:
 *      ((OfType 5003 ) (and) ((Equal 99 == 99) (and) (Equal Event-Field "servity" > 99))
 *
 */

static UA_StatusCode
return_number_of_operands(UA_FilterOperator oprt, int *num_filter_operands){
    if (oprt == 1 || oprt == 7 || oprt == 13 || oprt == 14){
        *num_filter_operands = 1;
        //input_value_array [num_filter_operands]
    }
    else if(oprt == 0 || oprt == 2 || oprt == 3 || oprt == 4 || oprt == 5 ||
            oprt == 6 || oprt == 10 || oprt == 11 || oprt == 12 || oprt == 16 ||
            oprt == 17){
        *num_filter_operands = 2;
    }
    else if(oprt == 8){
        *num_filter_operands = 3;
    }
    else if(oprt == 15){
        *num_filter_operands = 15;
    }
    else{
        return UA_STATUSCODE_BAD;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
auto_set_where_clause(UA_ContentFilter *contentFilter, UA_FilterOperator *FilterOperatorList,
                      size_t number_FilterOperators, const int *TypeList,
                      Literal_Operand_Comparison *literal_operand_values, UA_of_Type_Values *values){

    UA_UInt16 whereClauseSize = (UA_UInt16) number_FilterOperators;
    //set up the content filter
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
    int number_operands;
    for(size_t i=0; i< number_FilterOperators; ++i){
        if (FilterOperatorList[i] != UA_FILTEROPERATOR_INLIST){
            return_number_of_operands(FilterOperatorList[i], &number_operands);
            contentFilter->elements[i].filterOperator = FilterOperatorList[i];
            contentFilter->elements[i].filterOperandsSize = (size_t) number_operands;
            UA_StatusCode res = UA_STATUSCODE_GOOD;
            res = setupOperandArrays(contentFilter);
            if(res != UA_STATUSCODE_GOOD) {
                UA_ContentFilter_clear(contentFilter);
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
        else{
            printf("the number of operators has to be extracted from the input list\n");
            //write code to determine the length of the list...
        }
    }

    UA_String out = UA_STRING_NULL;
    //declare count variables
    UA_UInt32 element_operand_ctr = 1;
    int of_type_ctr = 0;
    int lit_oper_ctr = 0;
    //fill the elements
    for (size_t i =0; i< number_FilterOperators; ++i){
        //distinguish between the different cases and input values
        if(TypeList[i] == UA_TYPES_ELEMENTOPERAND && (FilterOperatorList[i] == UA_FILTEROPERATOR_OR || FilterOperatorList[i] == UA_FILTEROPERATOR_AND)){
            setupTwoOperandsFilter(&contentFilter->elements[i], element_operand_ctr, (element_operand_ctr+1));
            element_operand_ctr +=2;
        }
        else if(FilterOperatorList[i] == UA_FILTEROPERATOR_OFTYPE && TypeList[i] == UA_TYPES_LITERALOPERAND){
            setupOfTypeFilter(&contentFilter->elements[i], values[of_type_ctr].nsIndex, values[of_type_ctr].typeId);
                of_type_ctr += 1;
        }
        else if(TypeList[i] == UA_TYPES_LITERALOPERAND && contentFilter->elements[i].filterOperandsSize == 2){
            if(literal_operand_values[lit_oper_ctr].operand_one.content.decoded.type == literal_operand_values[lit_oper_ctr].operand_two.content.decoded.type){
                //check if both are literal values
                if (literal_operand_values[lit_oper_ctr].operand_one.content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND]){
                    printf("**************************** Both are literal values *********************************************\n");
                    contentFilter->elements[i].filterOperands[0] = literal_operand_values[lit_oper_ctr].operand_one;
                    contentFilter->elements[i].filterOperands[1] = literal_operand_values[lit_oper_ctr].operand_two;
                    lit_oper_ctr +=1;
                }
                else{
                    printf("**************************** Both have same types but are not literal types *********************************************\n");
                    lit_oper_ctr +=1;
                }
            }
            else if(literal_operand_values[lit_oper_ctr].operand_one.content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND]
                    && literal_operand_values[lit_oper_ctr].operand_two.content.decoded.type == &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]){
                printf("**************************** first is literal, second is SAO *********************************************\n");
                contentFilter->elements[i].filterOperands[0] = literal_operand_values[lit_oper_ctr].operand_one;
                contentFilter->elements[i].filterOperands[1] = literal_operand_values[lit_oper_ctr].operand_two;
                lit_oper_ctr +=1;
            }
            else{
                printf("**************************** unspecified case by distinguishing Literal Operands *********************************************\n");
                lit_oper_ctr +=1;
            }


        }
        else{
            printf("nothing added\n");
        }


    }


    UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
    printf("Full content Filter %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);

    return result;

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

static void
handler_events_filter(UA_Client *client, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Event Notification (Filter passed)");
    for(size_t i = 0; i < nEventFields; ++i) {
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            UA_UInt16 severity = *(UA_UInt16 *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message: '%.*s'", (int)lt->text.length, lt->text.data);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_NODEID])) {
            UA_String nodeIdName = UA_STRING_ALLOC("");
            UA_NodeId_print((UA_NodeId *)eventFields[i].data, &nodeIdName);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "TypeNodeId: '%.*s'", (int)nodeIdName.length, nodeIdName.data);
            UA_String_clear(&nodeIdName);
        } else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type: '%s'", eventFields[i].type->typeName);
#else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION "
                        "for typename");
#endif
        }
    }
}

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static void
setLitOperand_with_variant(UA_ExtensionObject *element,  UA_Variant literal){
    element->content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    element->encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(literalOperand);
    literalOperand->value = literal;
    element->content.decoded.data = literalOperand;
}





int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc < 2) {
        printf("Usage: tutorial_client_event_filter <opc.tcp://server-url>\n");
        return EXIT_FAILURE;
    }

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    UA_UInt32 subId = response.subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Create subscription succeeded, id %u", subId);

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    UA_QualifiedName qm[SELECT_CLAUSE_FIELD_COUNT];
    qm[0].namespaceIndex = 0;
    qm[0].name = UA_STRING("Message");
    qm[1].namespaceIndex = 0;
    qm[1].name = UA_STRING("Severity");
    qm[2].namespaceIndex = 0;
    qm[2].name = UA_STRING("EventType");
    filter.selectClauses = setupSelectClauses(SELECT_CLAUSE_FIELD_COUNT, qm);
    filter.selectClausesSize = SELECT_CLAUSE_FIELD_COUNT;
    retval = setupWhereClauses(&filter.whereClause, 5, 3);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    UA_String out = UA_STRING_NULL;


    printf("Auto setup\n");
    UA_EventFilter test_filter;
    UA_EventFilter_init(&test_filter);
    //set up select clause

    //case 0:
    //--------------------------------------------
    /*FilterOperatorList
    UA_FilterOperator FilterOperatorList[3] = {UA_FILTEROPERATOR_OR, UA_FILTEROPERATOR_OFTYPE, UA_FILTEROPERATOR_OFTYPE};
    FilterOperand Parameter List
    int TypeList[3] = {UA_TYPES_ELEMENTOPERAND, UA_TYPES_LITERALOPERAND, UA_TYPES_LITERALOPERAND};
    UA_of_Type_Values value_one;
    value_one.nsIndex = (UA_UInt16)0;
    value_one.typeId = (UA_UInt32) 5003;

    UA_of_Type_Values value_two;
    value_two.nsIndex = (UA_UInt16)0;
    value_two.typeId = (UA_UInt32) 3035;
    //ofType_OperatorList
    UA_of_Type_Values values[2] = {value_one, value_two};
    size_t number_FilterOperators = sizeof(FilterOperatorList)/sizeof(FilterOperatorList[0]);
    retval = auto_set_where_clause(&test_filter.whereClause, FilterOperatorList, number_FilterOperators, TypeList, NULL, values);*/
    //--------------------------------------------

    // case 1:
    //---------------------------------------------
    //FilterOperatorList
    /*UA_FilterOperator FilterOperatorList[1]= {UA_FILTEROPERATOR_OFTYPE};
    //FilterOperand Parameter List
    int TypeList[1] = {UA_TYPES_LITERALOPERAND};

    UA_of_Type_Values value_one;
    value_one.nsIndex = (UA_UInt16)0;
    value_one.typeId = (UA_UInt32) 5001;
     //List of values for the of_Type operator
    UA_of_Type_Values values[1] = {value_one};
     size_t number_FilterOperators = sizeof(FilterOperatorList)/sizeof(FilterOperatorList[0]);
    retval = auto_set_where_clause(&test_filter.whereClause, FilterOperatorList, number_FilterOperators, TypeList, NULL, values);*/
    //---------------------------------------------------

    //case 2:
    //-------------------------------------
    /*UA_FilterOperator FilterOperatorList[11]= {UA_FILTEROPERATOR_OR, UA_FILTEROPERATOR_OR,
                                               UA_FILTEROPERATOR_OR, UA_FILTEROPERATOR_OR,
                                               UA_FILTEROPERATOR_OR, UA_FILTEROPERATOR_OFTYPE,
                                               UA_FILTEROPERATOR_OFTYPE,UA_FILTEROPERATOR_OFTYPE,
                                               UA_FILTEROPERATOR_OFTYPE, UA_FILTEROPERATOR_OFTYPE,
                                               UA_FILTEROPERATOR_OFTYPE};
    //FilterOperand Parameter List
    int TypeList[11] = {UA_TYPES_ELEMENTOPERAND, UA_TYPES_ELEMENTOPERAND,
                        UA_TYPES_ELEMENTOPERAND, UA_TYPES_ELEMENTOPERAND,
                        UA_TYPES_ELEMENTOPERAND, UA_TYPES_LITERALOPERAND,
                        UA_TYPES_LITERALOPERAND, UA_TYPES_LITERALOPERAND,
                        UA_TYPES_LITERALOPERAND ,UA_TYPES_LITERALOPERAND,
                        UA_TYPES_LITERALOPERAND};

    UA_of_Type_Values value_one;
    value_one.nsIndex = (UA_UInt16)0;
    value_one.typeId = (UA_UInt32) 5000;

    UA_of_Type_Values value_two;
    value_two.nsIndex = (UA_UInt16)0;
    value_two.typeId = (UA_UInt32) 5001;

    UA_of_Type_Values value_three;
    value_three.nsIndex = (UA_UInt16)0;
    value_three.typeId = (UA_UInt32) 5002;

    UA_of_Type_Values value_four;
    value_four.nsIndex = (UA_UInt16)0;
    value_four.typeId = (UA_UInt32) 5003;

    UA_of_Type_Values value_five;
    value_five.nsIndex = (UA_UInt16)0;
    value_five.typeId = (UA_UInt32) 5004;

    UA_of_Type_Values value_six;
    value_six.nsIndex = (UA_UInt16)0;
    value_six.typeId = (UA_UInt32) 3035;

    //List of values for the of_Type operator
    UA_of_Type_Values values[6] = {value_one, value_two,
                                   value_three, value_four,
                                   value_five, value_six};
    size_t number_FilterOperators = sizeof(FilterOperatorList)/sizeof(FilterOperatorList[0]);
    retval = auto_set_where_clause(&test_filter.whereClause, FilterOperatorList, number_FilterOperators, TypeList, NULL, values);*/
    //-------------------------------------

    //case 3:
    //----------------------------------------
    //FilterOperatorList
    UA_FilterOperator FilterOperatorList[5]= { UA_FILTEROPERATOR_AND, UA_FILTEROPERATOR_OFTYPE,
                                               UA_FILTEROPERATOR_AND, UA_FILTEROPERATOR_EQUALS,
                                               UA_FILTEROPERATOR_GREATERTHAN};
    //FilterOperand Parameter List
    int TypeList[5] = {UA_TYPES_ELEMENTOPERAND, UA_TYPES_LITERALOPERAND,
                       UA_TYPES_ELEMENTOPERAND, UA_TYPES_LITERALOPERAND,
                       UA_TYPES_LITERALOPERAND};

    UA_of_Type_Values value_one;
    value_one.nsIndex = (UA_UInt16)0;
    value_one.typeId = (UA_UInt32) 5000;
    //List of values for the of_Type operator
    UA_of_Type_Values values[1] = {value_one};

    UA_Variant val1;
    UA_Variant_init(&val1);
    UA_UInt32 value1= 99;
    UA_Variant_setScalarCopy(&val1, &value1, &UA_TYPES[UA_TYPES_UINT32]);
    //list with literal values as UA_Variants
    //UA_Variant literal_values[3] = {val1, val1, val1};

    // distinguish different element combinations for the literal operand
    // -> compare literal operands to ech other,
    // ->compare literal operand to SAO,
    // ->compare literal operand to AO,
    Literal_Operand_Comparison case_one;
    setLitOperand_with_variant(&case_one.operand_one, val1);
    /*case_one.operand_one.encoding = UA_EXTENSIONOBJECT_DECODED;
    case_one.operand_one.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    case_one.operand_one.content.decoded.data = &val1;*/
    case_one.operand_two.encoding = UA_EXTENSIONOBJECT_DECODED;
    case_one.operand_two.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    case_one.operand_two.content.decoded.data = &val1;



    //declare SAO for the second example
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;
    sao.typeDefinitionId = UA_NODEID_NUMERIC(0, 5000);
    sao.browsePathSize = 1;
    UA_QualifiedName *qname = UA_QualifiedName_new();
    *qname = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    sao.browsePath = qname;

    Literal_Operand_Comparison case_two;
    case_two.operand_one.encoding = UA_EXTENSIONOBJECT_DECODED;
    case_two.operand_one.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    case_two.operand_one.content.decoded.data = &val1;
    case_two.operand_two.encoding = UA_EXTENSIONOBJECT_DECODED;
    case_two.operand_two.content.decoded.type = &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND];
    case_two.operand_two.content.decoded.data = &sao;
    //array to compare literal operands
    Literal_Operand_Comparison literal_operand_values[2] = {case_one, case_two};
    size_t number_FilterOperators = sizeof(FilterOperatorList)/sizeof(FilterOperatorList[0]);
    retval = auto_set_where_clause(&test_filter.whereClause, FilterOperatorList, number_FilterOperators, TypeList, literal_operand_values, values);
    //----------------------------------------

    //case 4:
    //----------------------------------------
    /*//FilterOperatorList
    UA_FilterOperator FilterOperatorList[3]= { UA_FILTEROPERATOR_AND, UA_FILTEROPERATOR_OFTYPE,
                                               UA_FILTEROPERATOR_GREATERTHAN};
    //FilterOperand Parameter List
    int TypeList[5] = {UA_TYPES_ELEMENTOPERAND, UA_TYPES_LITERALOPERAND,
                       UA_TYPES_LITERALOPERAND};
    */




    printf("Execute Test\n");

    UA_print(&test_filter.whereClause, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
    printf("Test Filter Where Clause: %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);

    printf("Example open62541\n");
    UA_print(&filter.whereClause, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
    printf("Where Clause: %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);




    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    UA_UInt32 monId = 0;
    UA_MonitoredItemCreateResult result =
        UA_Client_MonitoredItems_createEvent(client, subId,
                                             UA_TIMESTAMPSTORETURN_BOTH, item,
                                             &monId, handler_events_filter, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s", UA_StatusCode_name(result.statusCode));
        goto cleanup;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Server', id %u", response.subscriptionId);
    }

    monId = result.monitoredItemId;


    while(running)
        UA_Client_run_iterate(client, 100);

    /* Delete the subscription */
cleanup:
    UA_MonitoredItemCreateResult_clear(&result);
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    UA_Array_delete(filter.selectClauses, SELECT_CLAUSE_FIELD_COUNT, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    UA_Array_delete(filter.whereClause.elements, filter.whereClause.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
