#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"

static UA_Boolean satisfiesSignature(const UA_Variant *v, const UA_Argument *arg) {
    if(!UA_NodeId_equal(&v->type->typeId, &arg->dataType))
        return UA_FALSE;
    
    /* ValueRank Semantics
       n >= 1: the value is an array with the specified number of dimensions.
       n = 0: the value is an array with one or more dimensions.
       n = -1: the value is a scalar.
       n = -2: the value can be a scalar or an array with any number of dimensions.
       n = -3:  the value can be a scalar or a one dimensional array. */

    UA_Boolean scalar = UA_Variant_isScalar(v);
    if(arg->valueRank == 0 && scalar)
        return UA_FALSE;
    if(arg->valueRank == -1 && !scalar)
        return UA_FALSE;
    if(arg->valueRank == -3 && v->arrayDimensionsSize > 1)
        return UA_FALSE;
    if(arg->valueRank >= 1 && v->arrayDimensionsSize != arg->arrayDimensionsSize)
        return UA_FALSE;

    if(arg->arrayDimensionsSize >= 1) {
        if(arg->arrayDimensionsSize != v->arrayDimensionsSize)
            return UA_FALSE;
        for(UA_Int32 i = 0; i < arg->arrayDimensionsSize; i++) {
            if(arg->arrayDimensions[i] != v->arrayDimensions[i])
                return UA_FALSE;
        }
    }
    return UA_TRUE;
}

static void CallMethodRequest(UA_Server *server, UA_Session *session, const UA_CallMethodRequest *request,
                              UA_CallMethodResult *result) {

    /* Get the object and the method */
    const UA_ObjectNode *object = (const UA_ObjectNode*)UA_NodeStore_get(server->nodestore, &request->objectId);
    if(!object) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }
    if(object->nodeClass != UA_NODECLASS_OBJECT) {
        UA_NodeStore_release((const UA_Node*)object);
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    const UA_MethodNode *method = (const UA_MethodNode*)UA_NodeStore_get(server->nodestore, &request->methodId);
    if(!method) {
        UA_NodeStore_release((const UA_Node*)object);
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }
    if(method->nodeClass != UA_NODECLASS_METHOD) {
        UA_NodeStore_release((const UA_Node*)object);
        UA_NodeStore_release((const UA_Node*)method);
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }

    /* Check the arguments */
    if(request->inputArgumentsSize != method->inputArgumentsSize) {
        UA_NodeStore_release((const UA_Node*)object);
        UA_NodeStore_release((const UA_Node*)method);
        result->statusCode = UA_STATUSCODE_BADARGUMENTSMISSING; /* or too many arguments */
        return;
    }

    if(request->inputArgumentsSize > 0) {
        result->inputArgumentResults = UA_malloc(sizeof(UA_StatusCode) * request->inputArgumentsSize);
        if(!result->inputArgumentResults) {
            UA_NodeStore_release((const UA_Node*)object);
            UA_NodeStore_release((const UA_Node*)method);
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
    }
    result->inputArgumentResultsSize = request->inputArgumentsSize;
    
    for(UA_Int32 i = 0; i < request->inputArgumentsSize; i++) {
        if(!satisfiesSignature(&request->inputArguments[i], &method->inputArguments[i])) {
            UA_NodeStore_release((const UA_Node*)object);
            UA_NodeStore_release((const UA_Node*)method);
            result->inputArgumentResults[i] = UA_STATUSCODE_BADTYPEMISMATCH;
            result->statusCode = UA_STATUSCODE_BADTYPEMISMATCH;
            return;
        }
    }

    /* Prepare the output arguments */
    if(method->outputArgumentsSize > 0) {
        result->outputArguments = UA_Array_new(&UA_TYPES[UA_TYPES_VARIANT], method->outputArgumentsSize);
        if(!result->outputArguments) {
            UA_NodeStore_release((const UA_Node*)object);
            UA_NodeStore_release((const UA_Node*)method);
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
    }
    result->outputArgumentsSize = method->outputArgumentsSize;

    /* Call the method and return */
    // Todo: let the function return diagnostic infos to the input arguments
    result->statusCode = method->method(object->nodeId, request->inputArguments, result->outputArguments);
    UA_NodeStore_release((const UA_Node*)object);
    UA_NodeStore_release((const UA_Node*)method);
}

void Service_Call(UA_Server *server, UA_Session *session, const UA_CallRequest *request,
                  UA_CallResponse *response) {
    
    if(request->methodsToCallSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    
    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_CALLMETHODRESULT], request->methodsToCallSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->methodsToCallSize;
    
    for(UA_Int32 i = 0; i<request->methodsToCallSize; i++)
        CallMethodRequest(server, session, &request->methodsToCall[i], &response->results[i]);
}
