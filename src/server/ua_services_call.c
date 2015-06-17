#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"

#ifdef ENABLE_METHODCALLS

void Service_Call(UA_Server *server, UA_Session *session,
                  const UA_CallRequest *request,
                  UA_CallResponse *response) {
    
    if (request->methodsToCall == UA_NULL) return;
    
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNODECLASSINVALID;
    response->resultsSize=request->methodsToCallSize;
    response->results = (UA_CallMethodResult *) UA_malloc(sizeof(UA_CallMethodResult)*response->resultsSize);
    
    for(int i=0; i<request->methodsToCallSize;i++){
        UA_CallMethodRequest *rq = &request->methodsToCall[i];
        UA_CallMethodResult  *rs = &response->results[i];
        UA_CallMethodResult_init(rs);
        
        rs->inputArgumentDiagnosticInfosSize = 0;
        rs->inputArgumentResultsSize = 0;
        rs->outputArgumentsSize = 0;
        
        const UA_Node *methodCalled = UA_NodeStore_get(server->nodestore, &rq->methodId);
        if (methodCalled == UA_NULL) {
            rs->statusCode = UA_STATUSCODE_BADMETHODINVALID;
            continue;
        }
        const UA_Node *withObject   = UA_NodeStore_get(server->nodestore, &rq->objectId);
        if (withObject == UA_NULL) {
            rs->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
            printf("Obj not found\n");
            continue;
        }
        
        if (methodCalled->nodeClass != UA_NODECLASS_METHOD) {
            rs->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
            continue;
        }
        if (withObject->nodeClass != UA_NODECLASS_OBJECT && withObject->nodeClass != UA_NODECLASS_OBJECTTYPE) {
            rs->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
            printf("Obj not found 1\n");
            continue;
        }

        // Object must have a hasComponent reference (or any inherited referenceType from sayd reference) 
        // to be valid for a methodCall...
        for(int i=0; i<withObject->referencesSize; i++) {
            if(withObject->references[i].referenceTypeId.identifier.numeric == UA_NS0ID_HASCOMPONENT) {
                // FIXME: Not checking any subtypes of HasComponent at the moment
                if (UA_NodeId_equal(&withObject->references[i].targetId.nodeId, &methodCalled->nodeId)) {
                    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
                }
                
            }
        }
        UA_NodeStore_release(methodCalled);
        if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            return;
        }
        
        // Lookup Method Hook
        UA_MethodCall_Manager *manager = server->methodCallManager;
        UA_NodeAttachedMethod *hook = UA_MethodCallManager_getMethodByNodeId(manager, methodCalled->nodeId);
        
        UA_ArgumentsList *inArgs  = UA_ArgumentsList_new(rq->inputArgumentsSize, rq->inputArgumentsSize);
        UA_ArgumentsList *outArgs = UA_ArgumentsList_new(1, 0);
        
        inArgs->arguments  = rq->inputArguments;
        
        // Call method if available
        if (hook != NULL)
            hook->method(withObject, inArgs, outArgs);
        UA_NodeStore_release(withObject);
       
        rs->outputArgumentsSize = outArgs->argumentsSize;
        if (outArgs->argumentsSize > 0)
            rs->outputArguments = (UA_Variant *) UA_malloc(sizeof(UA_Variant) * outArgs->argumentsSize);
        for (unsigned int i=0; i < outArgs->argumentsSize; i++) {
            UA_Variant_copy(&outArgs->arguments[i], &rs->outputArguments[i]);
            UA_Variant_deleteMembers(&outArgs->arguments[i]);
        }
        
        UA_ArgumentsList_destroy(inArgs);
        UA_ArgumentsList_destroy(outArgs);
    }

    return;
}
#endif