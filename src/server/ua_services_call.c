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
    
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
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
                    rs->statusCode = UA_STATUSCODE_GOOD;
                }
                
            }
        }
        if ( rs->statusCode != UA_STATUSCODE_GOOD)
            continue;
        
        if(((const UA_MethodNode *) methodCalled)->executable     == UA_FALSE ||
           ((const UA_MethodNode *) methodCalled)->userExecutable == UA_FALSE ) {
            rs->statusCode = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
            continue;
        }
        
        UA_ArgumentsList *inArgs  = UA_ArgumentsList_new(rq->inputArgumentsSize, rq->inputArgumentsSize);
        for(unsigned int i=0; i<inArgs->argumentsSize; i++)
            UA_Variant_copy(&rq->inputArguments[i], &inArgs->arguments[i]);
        
        // Call method if available
        UA_ArgumentsList *outArgs;
        if (((const UA_MethodNode *) methodCalled)->attachedMethod.method != UA_NULL) {
            outArgs = UA_ArgumentsList_new(rq->inputArgumentsSize, 0);
            ((const UA_MethodNode *) methodCalled)->attachedMethod.method(withObject, inArgs, outArgs);
        }
        else {
            outArgs = UA_ArgumentsList_new(0, 0);
            outArgs->callResult = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
        }
        UA_NodeStore_release(withObject);
        UA_NodeStore_release(methodCalled);
        
        // Copy StatusCode and Argumentresults of the outputArguments
        rs->statusCode = outArgs->callResult;
        rs->inputArgumentResultsSize = outArgs->statusSize;
        if(outArgs->statusSize > 0)
            rs->inputArgumentResults = (UA_StatusCode *) UA_malloc(sizeof(UA_StatusCode) * outArgs->statusSize);
        for (unsigned int i=0; i < outArgs->statusSize; i++) {
            outArgs->status[i] = rs->inputArgumentResults[i];
        }
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