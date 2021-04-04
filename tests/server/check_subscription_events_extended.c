/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>

#include "testing_clock.h"
#include "thread_wrapper.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static UA_Server *server;
static size_t serverIterations;
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static MUTEX_HANDLE serverMutex;

UA_Client *client;

static UA_UInt32 subscriptionId;
static UA_NodeId eventType;
static size_t nSelectClauses = 4;
static UA_SimpleAttributeOperand *selectClauses;
static UA_SimpleAttributeOperand *selectClausesTest;
static UA_StatusCode *retvals;
static UA_ContentFilterResult *contentFilterResult;
static UA_ContentFilter *conFilter;
static UA_ContentFilter *whereClauses;
static UA_ContentFilterElement *elements;
static const size_t nWhereClauses = 1;
UA_Double publishingInterval = 500.0;

static void
addNewEventType(void) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");

    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);
    UA_LocalizedText_clear(&attr.displayName);
    UA_LocalizedText_clear(&attr.description);
}

static void
setupSelectClauses(void) {
    /* Check for severity (set manually), message (set manually), eventType
     * (automatic) and sourceNode (automatic) */
    selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return;

    for(size_t i = 0; i < nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
        selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        selectClauses[i].browsePathSize = 1;
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
        selectClauses[i].browsePath = (UA_QualifiedName *)
            UA_Array_new(selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        if(!selectClauses[i].browsePathSize) {
            UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        }
    }

    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventType");
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceNode");
}

static void setupWhereClauses(void){
    whereClauses = (UA_ContentFilter*)
        UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTER]);
    whereClauses->elementsSize= 1;
    whereClauses->elements  = (UA_ContentFilterElement*)
        UA_Array_new(whereClauses->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
    for(size_t i =0; i<whereClauses->elementsSize; ++i) {
        UA_ContentFilterElement_init(&whereClauses->elements[i]);
    }
    whereClauses->elements[0].filterOperator = UA_FILTEROPERATOR_OFTYPE;
    whereClauses->elements[0].filterOperandsSize = 1;
    whereClauses->elements[0].filterOperands = (UA_ExtensionObject*)
        UA_Array_new(whereClauses->elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if (!whereClauses->elements[0].filterOperands){
        UA_ContentFilter_clear(whereClauses);
    }
    whereClauses->elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    whereClauses->elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(literalOperand);
    UA_NodeId *nodeId = UA_NodeId_new();
    UA_NodeId_init(nodeId);
    nodeId->namespaceIndex = 0;
    nodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
    nodeId->identifier.numeric = UA_NS0ID_BASEEVENTTYPE;
    UA_Variant_setScalar(&literalOperand->value, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    whereClauses->elements[0].filterOperands[0].content.decoded.data = literalOperand;
}

// create a subscription and add a monitored item to it
static void
setupSubscription(void) {
    // Create subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    subscriptionId = response.subscriptionId;
}

static void
removeSubscription(void) {
    UA_DeleteSubscriptionsRequest deleteSubscriptionsRequest;
    UA_DeleteSubscriptionsRequest_init(&deleteSubscriptionsRequest);
    UA_UInt32 removeId = subscriptionId;
    deleteSubscriptionsRequest.subscriptionIdsSize = 1;
    deleteSubscriptionsRequest.subscriptionIds = &removeId;

    UA_DeleteSubscriptionsResponse deleteSubscriptionsResponse;
    UA_DeleteSubscriptionsResponse_init(&deleteSubscriptionsResponse);
    UA_LOCK(server->serviceMutex);
    Service_DeleteSubscriptions(server, &server->adminSession, &deleteSubscriptionsRequest,
                                &deleteSubscriptionsResponse);
    UA_UNLOCK(server->serviceMutex);
    UA_DeleteSubscriptionsResponse_clear(&deleteSubscriptionsResponse);
}

static void serverMutexLock(void) {
    if (!(MUTEX_LOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be locked.\n");
        exit(1);
    }
}

static void serverMutexUnlock(void) {
    if (!(MUTEX_UNLOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be unlocked.\n");
        exit(1);
    }
}

THREAD_CALLBACK(serverloop) {
    while (running) {
        serverMutexLock();
        UA_Server_run_iterate(server, false);
        serverIterations++;
        serverMutexUnlock();
    }
    return 0;
}

static void
sleepUntilAnswer(UA_Double sleepMs) {
    UA_fakeSleep((UA_UInt32)sleepMs);
    serverMutexLock();
    size_t oldIterations = serverIterations;
    size_t newIterations;
    serverMutexUnlock();
    while(true) {
        serverMutexLock();
        newIterations = serverIterations;
        serverMutexUnlock();
        if(oldIterations != newIterations)
            return;
        UA_realSleep(1);
    }
}

static void
setup(void) {
    if (!MUTEX_INIT(serverMutex)) {
        fprintf(stderr, "Server mutex was not created correctly.");
        exit(1);
    }
    running = true;

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);

    addNewEventType();
    setupSelectClauses();
    setupWhereClauses();
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s",
                UA_StatusCode_name(retval));
        exit(1);
    }
    setupSubscription();

    sleepUntilAnswer(publishingInterval + 100);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    removeSubscription();
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    if (!MUTEX_DESTROY(serverMutex)) {
        fprintf(stderr, "Server mutex was not destroyed correctly.");
        exit(1);
    }
}

static UA_StatusCode
eventSetup(UA_NodeId *eventNodeId) {
    UA_StatusCode retval;
    serverMutexLock();
    retval = UA_Server_createEvent(server, eventType, eventNodeId);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    // add a severity to the event
    UA_Variant value;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *eventNodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Severity");
    serverMutexLock();
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    serverMutexUnlock();
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    // number with no special meaning
    UA_UInt16 eventSeverity = 1000;
    UA_Variant_setScalar(&value, &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    serverMutexLock();
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);

    //add a message to the event
    rpe.targetName = UA_QUALIFIEDNAME(0, "Message");
    serverMutexLock();
    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    serverMutexUnlock();
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    serverMutexLock();
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);

    return retval;
}


static UA_ContentFilter *setupWhereClausesComplex(void){
    conFilter = (UA_ContentFilter*)
        UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTER]);
    for(size_t i =0; i<nWhereClauses; ++i) {
        UA_ContentFilter_init(&conFilter[i]);
    }
    conFilter[0].elementsSize = 13;
    conFilter[0].elements  = (UA_ContentFilterElement*)
        UA_Array_new(
            conFilter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
    for(size_t i =0; i< conFilter[0].elementsSize; ++i) {
        UA_ContentFilterElement_init(&conFilter[0].elements[i]);
    }
    conFilter[0].elements[0].filterOperator = UA_FILTEROPERATOR_AND; // set the first Operator
    conFilter[0].elements[1].filterOperator = UA_FILTEROPERATOR_OR; // set the second Operator
    conFilter[0].elements[2].filterOperator = UA_FILTEROPERATOR_INLIST; // set the third Operator
    conFilter[0].elements[3].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the fourth Operator
    conFilter[0].elements[4].filterOperator = UA_FILTEROPERATOR_AND; // set the fifth Operator
    conFilter[0].elements[5].filterOperator = UA_FILTEROPERATOR_AND; // set the sixth Operator
    conFilter[0].elements[6].filterOperator = UA_FILTEROPERATOR_AND; // set the seventh Operator
    conFilter[0].elements[7].filterOperator = UA_FILTEROPERATOR_EQUALS; // set the eighth Operator
    conFilter[0].elements[8].filterOperator = UA_FILTEROPERATOR_BITWISEAND; // set the ninth Operator
    conFilter[0].elements[9].filterOperator = UA_FILTEROPERATOR_NOT; // set the tenth Operator
    conFilter[0].elements[10].filterOperator = UA_FILTEROPERATOR_ISNULL ; // set the eleventh Operator
    conFilter[0].elements[11].filterOperator = UA_FILTEROPERATOR_GREATERTHAN; // set the twelfth Operator
    conFilter[0].elements[12].filterOperator = UA_FILTEROPERATOR_BETWEEN; // set the thirteenth Operator


    conFilter[0].elements[0].filterOperandsSize = 2;            // set Operands size of first Operator
    conFilter[0].elements[1].filterOperandsSize = 2;            // set Operands size of second Operator
    conFilter[0].elements[2].filterOperandsSize = 4;            // set Operands size of third Operator
    conFilter[0].elements[3].filterOperandsSize = 1;            // set Operands size of fourth Operator
    conFilter[0].elements[4].filterOperandsSize = 2;            // set Operands size of fifth Operator
    conFilter[0].elements[5].filterOperandsSize = 2;            // set Operands size of sixth Operator
    conFilter[0].elements[6].filterOperandsSize = 2;            // set Operands size of seventh Operator
    conFilter[0].elements[7].filterOperandsSize = 2;            // set Operands size of eighth Operator
    conFilter[0].elements[8].filterOperandsSize = 2;            // set Operands size of ninth Operator
    conFilter[0].elements[9].filterOperandsSize = 1;            // set Operands size of tenth Operator
    conFilter[0].elements[10].filterOperandsSize = 1;            // set Operands size of eleventh Operator
    conFilter[0].elements[11].filterOperandsSize = 2;            // set Operands size of twelfth Operator
    conFilter[0].elements[12].filterOperandsSize = 3;           // set Operands size of thirteenth Operator
    for(size_t i =0; i< conFilter[0].elementsSize; ++i) {  // Set Operands Arrays
        conFilter[0].elements[i].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(
                conFilter[0].elements[i].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        if (!conFilter[0].elements[i].filterOperands){
            UA_ContentFilter_clear(conFilter);
            return NULL;
        }
        for(size_t n =0; n< conFilter[0].elements[i].filterOperandsSize; ++n) {
            UA_ExtensionObject_init(&conFilter[0].elements[i].filterOperands[n]);
        }
    }

    // thirteenth Element  (24 UA_FILTEROPERATOR_BETWEEN (12 - 50))
    conFilter->elements[12].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[12].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[12].filterOperands[2].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    UA_LiteralOperand *firstLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(firstLiteralOperand);
    UA_LiteralOperand *secondLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(secondLiteralOperand);
    UA_LiteralOperand *thirdLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(thirdLiteralOperand);
    UA_Variant firstVariant;
    UA_Variant secondVariant;
    UA_Variant thirdVariant;
    firstVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    firstVariant.arrayDimensionsSize = 0;
    firstVariant.arrayDimensions = NULL;
    firstVariant.arrayLength = 0;
    firstVariant.storageType = UA_VARIANT_DATA;
    secondVariant.type = &UA_TYPES[UA_TYPES_UINT32];
    secondVariant.arrayDimensionsSize = 0;
    secondVariant.arrayDimensions = NULL;
    secondVariant.arrayLength = 0;
    secondVariant.storageType = UA_VARIANT_DATA;
    thirdVariant.type = &UA_TYPES[UA_TYPES_UINT32];
    thirdVariant.arrayDimensionsSize = 0;
    thirdVariant.arrayDimensions = NULL;
    thirdVariant.arrayLength = 0;
    thirdVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32 *value1 = UA_UInt32_new();
    UA_UInt32_init(value1);
    *value1 = 24;
    UA_UInt32  *value2 = UA_UInt32_new();
    UA_UInt32_init(value2);
    *value2 = 12;
    UA_UInt32  *value3 = UA_UInt32_new();
    UA_UInt32_init(value3);
    *value3 = 50;
    firstVariant.data =  value1;
    secondVariant.data =  value2;
    thirdVariant.data = value3;
    firstLiteralOperand->value = firstVariant;
    secondLiteralOperand->value = secondVariant;
    thirdLiteralOperand->value = thirdVariant;
    conFilter->elements[12].filterOperands[0].content.decoded.data = firstLiteralOperand;
    conFilter->elements[12].filterOperands[1].content.decoded.data = secondLiteralOperand;
    conFilter->elements[12].filterOperands[2].content.decoded.data = thirdLiteralOperand;

    // twelfth Element  (50 UA_FILTEROPERATOR_GREATERTHAN 24)
    UA_LiteralOperand *fifthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(fifthLiteralOperand);
    UA_LiteralOperand *sixthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(sixthLiteralOperand);
    UA_Variant fifthVariant;
    UA_Variant sixthVariant;
    fifthVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    fifthVariant.arrayDimensionsSize = 0;
    fifthVariant.arrayDimensions = NULL;
    fifthVariant.arrayLength = 0;
    fifthVariant.storageType = UA_VARIANT_DATA;
    sixthVariant.type = &UA_TYPES[UA_TYPES_UINT32];
    sixthVariant.arrayDimensionsSize = 0;
    sixthVariant.arrayDimensions = NULL;
    sixthVariant.arrayLength = 0;
    sixthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32 *value5 = UA_UInt32_new();
    UA_UInt32_init(value5);
    *value5 = 50;
    fifthVariant.data =  value5;
    UA_UInt32  *value6 = UA_UInt32_new();
    UA_UInt32_init(value6);
    *value6 = 24;
    sixthVariant.data =  value6;
    fifthLiteralOperand->value = fifthVariant;
    sixthLiteralOperand->value = sixthVariant;
    conFilter->elements[11].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[11].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[11].filterOperands[0].content.decoded.data = fifthLiteralOperand;
    conFilter->elements[11].filterOperands[1].content.decoded.data = sixthLiteralOperand;

    //  eleventh Element (UA_FILTEROPERATOR_ISNULL)
    conFilter->elements[10].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    UA_LiteralOperand *twelfthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(twelfthLiteralOperand);
    UA_Variant twelfthVariant;
    twelfthVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    twelfthVariant.arrayDimensionsSize = 0;
    twelfthVariant.arrayDimensions = NULL;
    twelfthVariant.arrayLength = 0;
    twelfthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32  *value12 = UA_UInt32_new();
    UA_UInt32_init(value12);
    *value12 = 24;
    twelfthVariant.data =  value12;
    twelfthLiteralOperand->value = twelfthVariant;
    conFilter->elements[10].filterOperands[0].content.decoded.data = twelfthLiteralOperand;

    // tenth Element (UA_FILTEROPERATOR_NOT)
    conFilter->elements[9].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[9].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *elementOperand;
    elementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(elementOperand);
    elementOperand->index = 10;
    conFilter->elements[9].filterOperands[0].content.decoded.data = elementOperand;

    // ninth Element ( 24 UA_FILTEROPERATOR_BITWISEAND 24 )
    UA_LiteralOperand *seventhLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(seventhLiteralOperand);
    UA_LiteralOperand *eighthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(eighthLiteralOperand);
    UA_Variant seventhVariant;
    UA_Variant eighthVariant;
    seventhVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    seventhVariant.arrayDimensionsSize = 0;
    seventhVariant.arrayDimensions = NULL;
    seventhVariant.arrayLength = 0;
    seventhVariant.storageType = UA_VARIANT_DATA;
    eighthVariant.type = &UA_TYPES[UA_TYPES_UINT32];
    eighthVariant.arrayDimensionsSize = 0;
    eighthVariant.arrayDimensions = NULL;
    eighthVariant.arrayLength = 0;
    eighthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32 *value7 = UA_UInt32_new();
    UA_UInt32_init(value7);
    *value7 = 24;
    UA_UInt32  *value8 = UA_UInt32_new();
    UA_UInt32_init(value8);
    *value8 = 24;
    seventhVariant.data =  value7;
    eighthVariant.data =  value8;
    seventhLiteralOperand->value = seventhVariant;
    eighthLiteralOperand->value = eighthVariant;
    conFilter->elements[8].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[8].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[8].filterOperands[0].content.decoded.data = seventhLiteralOperand;
    conFilter->elements[8].filterOperands[1].content.decoded.data = eighthLiteralOperand;

    // eighth Element (UA_FILTEROPERATOR_EQUALS)
    conFilter->elements[7].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[7].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    UA_LiteralOperand *elemLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(elemLiteralOperand);
    UA_ElementOperand *indexOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(indexOperand);
    UA_Variant Variant;
    Variant.type = &UA_TYPES[UA_TYPES_UINT16];
    Variant.arrayDimensionsSize = 0;
    Variant.arrayDimensions = NULL;
    Variant.arrayLength = 0;
    Variant.storageType = UA_VARIANT_DATA;
    UA_UInt32 *value15 = UA_UInt32_new();
    UA_UInt32_init(value15);
    *value15 = 24;
    Variant.data =  value15;
    elemLiteralOperand->value = Variant;
    indexOperand->index = 8;
    conFilter->elements[7].filterOperands[0].content.decoded.data = elemLiteralOperand;
    conFilter->elements[7].filterOperands[1].content.decoded.data = indexOperand;

    // seventh Element (UA_FILTEROPERATOR_AND)
    conFilter->elements[6].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[6].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter->elements[6].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[6].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *secondElementOperand;
    secondElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(secondElementOperand);
    secondElementOperand->index = 7;
    UA_ElementOperand *thirdElementOperand;
    thirdElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(thirdElementOperand);
    thirdElementOperand->index = 9;
    conFilter->elements[6].filterOperands[0].content.decoded.data = secondElementOperand;
    conFilter->elements[6].filterOperands[1].content.decoded.data = thirdElementOperand;

    // sixth Element (UA_FILTEROPERATOR_AND)
    conFilter->elements[5].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[5].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter->elements[5].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[5].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *fourthElementOperand;
    fourthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(fourthElementOperand);
    fourthElementOperand->index = 6;
    UA_ElementOperand *fifthElementOperand;
    fifthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(fifthElementOperand);
    fifthElementOperand->index = 11;
    conFilter->elements[5].filterOperands[0].content.decoded.data = fourthElementOperand;
    conFilter->elements[5].filterOperands[1].content.decoded.data = fifthElementOperand;

    // fifth Element (UA_FILTEROPERATOR_AND)
    conFilter->elements[4].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[4].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter->elements[4].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[4].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *sixthElementOperand;
    sixthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(sixthElementOperand);
    sixthElementOperand->index = 5;
    UA_ElementOperand *seventhElementOperand;
    seventhElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(seventhElementOperand);
    seventhElementOperand->index = 12;
    conFilter->elements[4].filterOperands[0].content.decoded.data = sixthElementOperand;
    conFilter->elements[4].filterOperands[1].content.decoded.data = seventhElementOperand;

    // fourth Element (UA_FILTEROPERATOR_OFTYPE)
    conFilter->elements[3].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(literalOperand);
    UA_NodeId *nodeId = UA_NodeId_new();
    UA_NodeId_init(nodeId);
    nodeId->namespaceIndex = 0;
    nodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
    nodeId->identifier.numeric = UA_NS0ID_BASEEVENTTYPE;
    UA_Variant_setScalar(&literalOperand->value, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    conFilter->elements[3].filterOperands[0].content.decoded.data = literalOperand;

    // Third Element (24 UA_FILTEROPERATOR_INLIST (12,50,70))
    conFilter->elements[2].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[2].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[2].filterOperands[2].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    conFilter->elements[2].filterOperands[3].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    UA_LiteralOperand *fourthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(fourthLiteralOperand);
    UA_Variant fourthVariant;
    fourthVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    fourthVariant.arrayDimensionsSize = 0;
    fourthVariant.arrayDimensions = NULL;
    fourthVariant.arrayLength = 0;
    fourthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32  *value4 = UA_UInt32_new();
    UA_UInt32_init(value4);
    *value4 = 24;
    fourthVariant.data =  value4;
    fourthLiteralOperand->value = fourthVariant;
    UA_LiteralOperand *ninthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(ninthLiteralOperand);
    UA_Variant ninthVariant;
    ninthVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    ninthVariant.arrayDimensionsSize = 0;
    ninthVariant.arrayDimensions = NULL;
    ninthVariant.arrayLength = 0;
    ninthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32  *value9 = UA_UInt32_new();
    UA_UInt32_init(value9);
    *value9 = 12;
    ninthVariant.data =  value9;
    ninthLiteralOperand->value = ninthVariant;
    UA_LiteralOperand *tenthLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(tenthLiteralOperand);
    UA_Variant tenthVariant;
    tenthVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    tenthVariant.arrayDimensionsSize = 0;
    tenthVariant.arrayDimensions = NULL;
    tenthVariant.arrayLength = 0;
    tenthVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32  *value10 = UA_UInt32_new();
    UA_UInt32_init(value10);
    *value10 = 50;
    tenthVariant.data =  value10;
    tenthLiteralOperand->value = tenthVariant;
    UA_LiteralOperand *eleventhLiteralOperand = UA_LiteralOperand_new();
    UA_LiteralOperand_init(eleventhLiteralOperand);
    UA_Variant eleventhVariant;
    eleventhVariant.type = &UA_TYPES[UA_TYPES_UINT16];
    eleventhVariant.arrayDimensionsSize = 0;
    eleventhVariant.arrayDimensions = NULL;
    eleventhVariant.arrayLength = 0;
    eleventhVariant.storageType = UA_VARIANT_DATA;
    UA_UInt32  *value11 = UA_UInt32_new();
    UA_UInt32_init(value11);
    *value11 = 70;
    eleventhVariant.data =  value11;
    eleventhLiteralOperand->value = eleventhVariant;
    conFilter->elements[2].filterOperands[0].content.decoded.data = fourthLiteralOperand;
    conFilter->elements[2].filterOperands[1].content.decoded.data = ninthLiteralOperand;
    conFilter->elements[2].filterOperands[2].content.decoded.data = tenthLiteralOperand;
    conFilter->elements[2].filterOperands[3].content.decoded.data = eleventhLiteralOperand;

    // second Element ( UA_FILTEROPERATOR_OR)
    conFilter->elements[1].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[1].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter->elements[1].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[1].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *eighthElementOperand;
    eighthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(eighthElementOperand);
    eighthElementOperand->index = 2;
    UA_ElementOperand *ninthElementOperand;
    ninthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(ninthElementOperand);
    ninthElementOperand->index = 3;
    conFilter->elements[1].filterOperands[0].content.decoded.data = eighthElementOperand;
    conFilter->elements[1].filterOperands[1].content.decoded.data = ninthElementOperand;

    //First Element (UA_FILTEROPERATOR_AND)
    conFilter->elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter->elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter->elements[0].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_ElementOperand *tenthElementOperand;
    tenthElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(tenthElementOperand);
    tenthElementOperand->index = 1;
    UA_ElementOperand *eleventhElementOperand;
    eleventhElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(eleventhElementOperand);
    eleventhElementOperand->index = 4;
    conFilter->elements[0].filterOperands[0].content.decoded.data = tenthElementOperand;
    conFilter->elements[0].filterOperands[1].content.decoded.data = eleventhElementOperand;
    return conFilter;
}

START_TEST(evaluateWhereClause){
        UA_ContentFilter *contentFilter = setupWhereClausesComplex();  // Filter Structure  (YARRAYITEMTYPE) OR (ProgressEventType)
        UA_NodeId eventNodeId;
        UA_Session session;
        UA_StatusCode retval = eventSetup(&eventNodeId);    // Event Setup (BaseEventType)
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        /* init contentFilterResult */
        UA_ContentFilterResult *contentFilterRes = (UA_ContentFilterResult*)
            UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTERRESULT]);
        contentFilterRes[0].elementResultsSize = contentFilter->elementsSize;
        contentFilterRes[0].elementResults  = (UA_ContentFilterElementResult *)
            UA_Array_new(contentFilterRes[0].elementResultsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT] );
        for(size_t i =0; i<contentFilterRes[0].elementResultsSize; ++i) {
            UA_ContentFilterElementResult_init(&contentFilterRes[0].elementResults[i]);
            contentFilterRes[0].elementResults[i].operandStatusCodes = (UA_StatusCode *)
                UA_Array_new(contentFilter[0].elements->filterOperandsSize,&UA_TYPES[UA_TYPES_STATUSCODE]);
        }
        UA_LOCK(server->serviceMutex);
        UA_StatusCode  res = UA_Server_startWhereClauseEvaluation(server,&session,&eventNodeId,contentFilter,contentFilterRes);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        UA_ContentFilterResult_delete(contentFilterRes);
}
END_TEST

START_TEST(initialWhereClauseValidation) {
        /* Everything is on the stack, so no memory cleaning required.*/
//        UA_NodeId eventNodeId;
        UA_ContentFilter contentFilter;
        contentFilter.elementsSize = 0;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        contentFilter.elements = elements;
        /* Empty Filter */
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_ptr_eq(contentFilterResult->elementResults,NULL);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
        contentFilter.elementsSize = 1;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements = elements;
        /* Illegal filter operators */
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_RELATEDTO;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_INVIEW;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);

        /*************************** UA_FILTEROPERATOR_OR ***************************/
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OR;
        /* No operand provided */
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands size */
        contentFilter.elements[0].filterOperandsSize = 1;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands type */
        contentFilter.elements[0].filterOperandsSize = 2;
        contentFilter.elements[0].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(contentFilter.elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        // Illegal filter operands INDEXRANGE
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
        UA_Array_delete(elements,contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        /* init */
        contentFilter.elementsSize = 3;
        elements = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements  = elements;
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_init(&contentFilter.elements[i]);
        }
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OR; // set the first Operator
        contentFilter.elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the second Operator
        contentFilter.elements[2].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the third Operator
        contentFilter.elements[0].filterOperandsSize = 2;            // set Operands size of first Operator
        contentFilter.elements[1].filterOperandsSize = 1;            // set Operands size of second Operator
        contentFilter.elements[2].filterOperandsSize = 1;            // set Operands size of third Operator
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {  // Set Operands Arrays
            contentFilter.elements[i].filterOperands = (UA_ExtensionObject*)
                UA_Array_new(contentFilter.elements[i].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
            for(size_t n =0; n<contentFilter.elements[i].filterOperandsSize; ++n) {
                UA_ExtensionObject_init(&contentFilter.elements[i].filterOperands[n]);
            }
        }
        // Second Element
        contentFilter.elements[1].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        contentFilter.elements[1].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
        UA_LiteralOperand_init(literalOperand);
        UA_NodeId *nodeId = UA_NodeId_new();
        UA_NodeId_init(nodeId);
        nodeId->namespaceIndex = 0;
        nodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
        nodeId->identifier.numeric = UA_NS0ID_AUDITEVENTTYPE;
        UA_Variant_setScalar(&literalOperand->value, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        contentFilter.elements[1].filterOperands[0].content.decoded.data = literalOperand;
        // Third Element
        contentFilter.elements[2].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        contentFilter.elements[2].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_LiteralOperand *secondliteralOperand = UA_LiteralOperand_new();
        UA_LiteralOperand_init(secondliteralOperand);
        UA_NodeId *secondnodeId = UA_NodeId_new();
        UA_NodeId_init(secondnodeId);
        secondnodeId->namespaceIndex = 0;
        secondnodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
        secondnodeId->identifier.numeric = UA_NS0ID_BASEEVENTTYPE;
        UA_Variant_setScalar(&secondliteralOperand->value, secondnodeId, &UA_TYPES[UA_TYPES_NODEID]);
        contentFilter.elements[2].filterOperands[0].content.decoded.data = secondliteralOperand;
        //First Element
        UA_ElementOperand *elementOperand;
        elementOperand = UA_ElementOperand_new();
        elementOperand->index = 1;
        UA_ElementOperand *secondElementOperand;
        secondElementOperand = UA_ElementOperand_new();
        secondElementOperand->index = 3;      // invalid index
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = elementOperand;
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
        contentFilter.elements[0].filterOperands[1].content.decoded.data = secondElementOperand;
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADINDEXRANGEINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /*clear */
        UA_Array_delete(elements,contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        /*************************** UA_FILTEROPERATOR_OFTYPE **************************/
        /* init */
        contentFilter.elementsSize = 1;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements = elements;
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_init(&contentFilter.elements[i]);
        }
        contentFilter.elements[0].filterOperandsSize = 0;
        // No operand provided
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OFTYPE;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands size */
        contentFilter.elements[0].filterOperandsSize = 2;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands type */
        contentFilter.elements[0].filterOperandsSize = 1;
        contentFilter.elements[0].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(contentFilter.elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands identifierType */
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        contentFilter.elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_LiteralOperand *thirdliteralOperand = UA_LiteralOperand_new();
        UA_LiteralOperand_init(thirdliteralOperand);
        UA_NodeId *thirdnodeId = UA_NodeId_new();
        UA_NodeId_init(thirdnodeId);
        thirdnodeId->namespaceIndex = 0;
        thirdnodeId->identifierType = UA_NODEIDTYPE_BYTESTRING;
        thirdnodeId->identifier.numeric = UA_NS0ID_AUDITEVENTTYPE;
        UA_Variant_setScalar(&thirdliteralOperand->value, thirdnodeId, &UA_TYPES[UA_TYPES_NODEID]);
        contentFilter.elements[0].filterOperands[0].content.decoded.data = thirdliteralOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands EventTypeId */
        thirdnodeId->identifierType = UA_NODEIDTYPE_NUMERIC;
        thirdnodeId->identifier.numeric = UA_NODEIDTYPE_NUMERIC;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = thirdliteralOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADNODEIDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Filter operands EventTypeId is a subtype of BaseEventType */
        thirdnodeId->identifier.numeric = UA_NS0ID_BASEEVENTTYPE;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = thirdliteralOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_GOOD);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
    }
END_TEST


START_TEST(validateSelectClause) {
/* Everything is on the stack, so no memory cleaning required.*/
        UA_StatusCode *retval;
        UA_EventFilter eventFilter;
        UA_EventFilter_init(&eventFilter);
        retval = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_ptr_eq(retval,NULL);
        UA_StatusCode_delete(retval);
        selectClausesTest = (UA_SimpleAttributeOperand *)
            UA_Array_new(7, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        eventFilter.selectClausesSize = 0;
        eventFilter.selectClauses = selectClausesTest;
        retval = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_ptr_eq(retval,NULL);
        UA_StatusCode_delete(retval);
        eventFilter.selectClausesSize = 7;
/*Initialization*/
        for(int i = 0; i < 7; i++){
            selectClausesTest[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
            selectClausesTest[i].browsePathSize = 1;
            selectClausesTest[i].browsePath = (UA_QualifiedName*)
                UA_Array_new(eventFilter.selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
            selectClausesTest[i].browsePath[0] = UA_QUALIFIEDNAME(0, "Test");
            selectClausesTest[i].attributeId = UA_ATTRIBUTEID_VALUE;
        }
/*typeDefinitionId not subtype of BaseEventType*/
        selectClausesTest[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_NUMBER);
/*attributeId not valid*/
        selectClausesTest[1].attributeId = 28;
/*browsePath contains null*/
        selectClausesTest[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "");
/*indexRange not valid*/
        selectClausesTest[3].indexRange = UA_STRING("test");
/*attributeId not value when indexRange is set*/
        selectClausesTest[4].attributeId = UA_ATTRIBUTEID_DATATYPE;
        selectClausesTest[4].indexRange = UA_STRING("1");
/*attributeId not value (should return UA_STATUSCODE_GOOD)*/
        selectClausesTest[5].attributeId = UA_ATTRIBUTEID_DATATYPE;
        eventFilter.selectClauses = selectClausesTest;
        retvals = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_uint_eq(retvals[0], UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
        ck_assert_uint_eq(retvals[1], UA_STATUSCODE_BADATTRIBUTEIDINVALID);
        ck_assert_uint_eq(retvals[2], UA_STATUSCODE_BADBROWSENAMEINVALID);
        ck_assert_uint_eq(retvals[3], UA_STATUSCODE_BADINDEXRANGEINVALID);
        ck_assert_uint_eq(retvals[4], UA_STATUSCODE_BADTYPEMISMATCH);
        ck_assert_uint_eq(retvals[5], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(retvals[6], UA_STATUSCODE_GOOD);
    }
END_TEST


#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

/* Assumes subscriptions work fine with data change because of other unit test */
static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription Events");
    TCase *tc_server = tcase_create("Server Subscription Events");
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, initialWhereClauseValidation);
    tcase_add_test(tc_server, validateSelectClause);
    tcase_add_test(tc_server, evaluateWhereClause);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
    suite_add_tcase(s, tc_server);

    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
