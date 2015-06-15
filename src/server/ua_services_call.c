#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_subscription.h"
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
            rs->statusCode = UA_STATUSCODE_BADMETHODINVALID;
            continue;
        }
        if (withObject->nodeClass != UA_NODECLASS_OBJECT && withObject->nodeClass != UA_NODECLASS_OBJECTTYPE) {
            rs->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
            printf("Obj not found 1\n");
            continue;
        }
        
    }
    
    
    return;
}
#endif