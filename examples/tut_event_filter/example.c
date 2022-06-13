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
//number of return values
#define SELECT_CLAUSE_FIELD_COUNT 3

/**********************
* Setting Up select clause
***********************/

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
//->See spec 4  P121 different func
//browsepathsize, ->nodeId as input
//indecRange is missing, as it is not part of the examples
/*static UA_AttributeOperand *
    setupselectClausesAttrOperand(size_t selectedFieldsSize, UA_NodeId *NodeIds, UA_RelativePath *BrowsePath, UA_String *index){
    UA_AttributeOperand *selectClauses = (UA_AttributeOperand*)
        UA_Array_new(selectedFieldsSize, &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]);
    if(!selectClauses){
        return NULL;
    }
    for(size_t i=0; i<selectedFieldsSize; ++i){
        UA_AttributeOperand_init(&selectClauses[i]);
    }
    for(size_t i=0; i<selectedFieldsSize; ++i){
        selectClauses[i].nodeId = NodeIds[i];
        selectClauses[i].browsePath = BrowsePath[i];
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }
    return selectClauses;
}
*/

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
setupOFTypeFilter(UA_ContentFilterElement *element, UA_UInt16 nsIndex, UA_UInt32 typeId){
    UA_String out = UA_STRING_NULL;
    UA_print(&*element, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT], &out);
    printf("%.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);
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
    UA_print(&*element, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT], &out);
    printf("%.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);
}
//only 4 operands are respected 
static void
setupRelatedToFilter(UA_ContentFilterElement *element, UA_Boolean nestedRelatedTo, UA_AttributeOperand firstOperand,
                     UA_UInt32 secondOperandIndex, UA_AttributeOperand secondOperand, UA_AttributeOperand thirdOperand,
                     UA_Int16 literal){

    UA_AttributeOperand *element_one = UA_AttributeOperand_new();
    *element_one = firstOperand;
    UA_AttributeOperand *element_two = UA_AttributeOperand_new();
    *element_two = secondOperand;
    UA_UInt32 *index = UA_UInt32_new();
    *index = secondOperandIndex;
    UA_AttributeOperand *three = UA_AttributeOperand_new();
    *three = thirdOperand;
    UA_Int16 *lit = UA_Int16_new();
    *lit = literal;
    //Alternatively use pre defined function -> //UA_ExtensionObject_setValue(&element->filterOperands[0], element_one, &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]);
    element->filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
    element->filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    element->filterOperands[0].content.decoded.data = element_one;
    //distinc between element ElementOperand and Attribute Operand -> boolean variable
    if(nestedRelatedTo == UA_TRUE){
        UA_ExtensionObject_setValue(&element->filterOperands[1], index, &UA_TYPES[UA_TYPES_ELEMENTOPERAND]);
    }else{
        UA_ExtensionObject_setValue(&element->filterOperands[1], element_two, &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]);
    }
    UA_ExtensionObject_setValue(&element->filterOperands[2], three, &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]);
    UA_ExtensionObject_setValue(&element->filterOperands[3], lit, &UA_TYPES[UA_TYPES_UINT16]);
    UA_String out = UA_STRING_NULL;
    UA_print(element, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT], &out);
    printf("yyy %.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);

}
/*
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
*/

/********************
change function, here only a relative path element is created
**************/
/*static UA_RelativePath
//ggf isInverse als input
setup_relativePath( UA_NodeId reference_type_Id, UA_QualifiedName targetName, size_t element_size, bool includesSubtypes){
    UA_RelativePathElement relpaele;
    UA_RelativePathElement_init(&relpaele);
    relpaele.referenceTypeId = reference_type_Id;
    relpaele.isInverse = false;
    relpaele.includeSubtypes = includesSubtypes;
    relpaele.targetName = targetName;
    
    //define relative path
    UA_RelativePath rel_pa;
    UA_RelativePath_init(&rel_pa);
    rel_pa.elementsSize = element_size;
    rel_pa.elements = &relpaele;
    return rel_pa;
}
*/
/***************************
 * where clauses
 *****************************/
static UA_StatusCode
setupWhereClauses(UA_ContentFilter *contentFilter, UA_UInt16 whereClauseSize, UA_UInt16 filterSelection){
    UA_ContentFilter_init(contentFilter);
    contentFilter->elementsSize = whereClauseSize;
    contentFilter->elements  = (UA_ContentFilterElement *)UA_Array_new(contentFilter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
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
            setupOFTypeFilter(&contentFilter->elements[1], 1, 5003);
            /* third Element (OfType) */
            setupOFTypeFilter(&contentFilter->elements[2], 0,
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
            printf("content filter\n");
            UA_String out = UA_STRING_NULL;
            UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);

            setupOFTypeFilter(&contentFilter->elements[0], 1, 5001);
            
            UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
            break;
        }
        //UA_Spec 4 Annex B Content Filter Example 1
        case 2: {

            contentFilter->elements[0].filterOperator = UA_FILTEROPERATOR_RELATEDTO;
            contentFilter->elements[1].filterOperator = UA_FILTEROPERATOR_RELATEDTO;
            contentFilter->elements[0].filterOperandsSize = 4;
            contentFilter->elements[1].filterOperandsSize = 4;
            result = setupOperandArrays(contentFilter);
            if(result != UA_STATUSCODE_GOOD){
                UA_ContentFilter_clear(contentFilter);
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
            printf("content filter\n");
            UA_String out = UA_STRING_NULL;
            UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);

            UA_AttributeOperand OperandOne;
            UA_AttributeOperand_init(&OperandOne);
            OperandOne.nodeId = UA_NODEID_NUMERIC(2, 1000);
            //OperandOne.browsePath = setup_relativePath(UA_NODEID_NUMERIC(0, 58),UA_QUALIFIEDNAME(2, "PersonType"),1, false);
            OperandOne.attributeId = 1;
            
            UA_Int16 Lit_Value = 1;
            UA_Variant Lit_Operand_value;
            UA_Variant_init(&Lit_Operand_value);
            UA_Variant_setScalarCopy(&Lit_Operand_value, &Lit_Value, &UA_TYPES[UA_TYPES_INT16]);
            
            UA_AttributeOperand OperandTwo;
            UA_AttributeOperand_init(&OperandTwo);
            
            UA_AttributeOperand OperandThree;
            UA_AttributeOperand_init(&OperandThree);
            OperandThree.nodeId = UA_NODEID_NUMERIC(2, 4004);
            //OperandThree.browsePath = setup_relativePath(UA_NODEID_NUMERIC(0, 31), UA_QUALIFIEDNAME(2, "HasPet"), 3, false);
            OperandThree.attributeId = 1;

            UA_Boolean nested_related_to = UA_TRUE;
            
            UA_UInt32 index = 2;
            //printf("set up first element\n");
            setupRelatedToFilter(&contentFilter->elements[0], nested_related_to, OperandOne, index, OperandTwo,
                                OperandThree, Lit_Value);

            printf("contentfilter first branch\n");
            UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);

            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);

            //second branch of the tree
            UA_AttributeOperand Operand_One;
            UA_AttributeOperand_init(&Operand_One);
            Operand_One.nodeId = UA_NODEID_NUMERIC(2, 1001);
            //Operand_One.browsePath = setup_relativePath(UA_NODEID_NUMERIC(0, 58), UA_QUALIFIEDNAME(2, "AnimalType"), 1, false);
            Operand_One.attributeId = 1;

            UA_AttributeOperand Operand_Two;
            UA_AttributeOperand_init(&Operand_Two);
            Operand_Two.nodeId = UA_NODEID_NUMERIC(2, 1005);
            //Operand_Two.browsePath = setup_relativePath(UA_NODEID_NUMERIC(0, 58), UA_QUALIFIEDNAME(2, "ScheduleType"), 1, false );
            Operand_Two.attributeId = 1;

            UA_AttributeOperand Operand_Three;
            UA_AttributeOperand_init(&Operand_Three);
            Operand_Three.nodeId = UA_NODEID_NUMERIC(2, 1005);
            //Operand_Three.browsePath = setup_relativePath(UA_NODEID_NUMERIC(0, 31), UA_QUALIFIEDNAME(2, "HasSchedule"), 2, true);
            Operand_Three.attributeId = 1;
            
            UA_Boolean nested_rel = UA_FALSE;
            UA_UInt32 opIndex = 0;
            setupRelatedToFilter(&contentFilter->elements[1], nested_rel, Operand_One, opIndex, Operand_Two, Operand_Three, Lit_Value);
            printf("contentfilter set: \n");
            UA_print(contentFilter, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
            break;
        }
        default:
            UA_ContentFilter_clear(contentFilter);
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}
#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
static void
handler_events_filter(UA_Client *client, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Event Notification (Filter passed)");
    UA_String out = UA_STRING_NULL;
    for(size_t i = 0; i < nEventFields; ++i) {
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            UA_UInt16 severity = *(UA_UInt16 *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Severity: %u", severity);
            UA_print(&severity,   &UA_TYPES[UA_TYPES_UINT16], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText *)eventFields[i].data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message: '%.*s'", (int)lt->text.length, lt->text.data);
            UA_print(&*lt,   &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
        } else if (UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_NODEID])) {
            UA_String nodeIdName = UA_STRING_ALLOC("");
            UA_NodeId_print((UA_NodeId *)eventFields[i].data, &nodeIdName);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "TypeNodeId: '%.*s'", (int)nodeIdName.length, nodeIdName.data);
            UA_String_clear(&nodeIdName);
            UA_print(&nodeIdName,   &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &out);
            printf("%.*s\n", (int)out.length, out.data);
            UA_String_clear(&out);
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
    printf("Monitored Item \n");

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    UA_QualifiedName qm[SELECT_CLAUSE_FIELD_COUNT];
    qm[0].namespaceIndex = 0;
    qm[0].name = UA_STRING("Message");
    qm[1].namespaceIndex = 0;
    qm[1].name = UA_STRING("Severity");
    qm[2].namespaceIndex = 0;
    qm[2].name = UA_STRING("EventType");
    printf("set uo filter\n");
    filter.selectClauses = setupSelectClauses(SELECT_CLAUSE_FIELD_COUNT, qm);
    filter.selectClausesSize = SELECT_CLAUSE_FIELD_COUNT;
    printf("set up content Filter\n");
    retval = setupWhereClauses(&filter.whereClause, 2, 2);
    
    printf("print content Filter\n");
    UA_String out = UA_STRING_NULL;
    UA_print(&filter.whereClause, &UA_TYPES[UA_TYPES_CONTENTFILTER], &out);
    printf("%.*s\n", (int)out.length, out.data);
    UA_String_clear(&out);
    printf("print content Filter\n");
    
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

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
        UA_Client_run_iterate(client, 10000);

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






