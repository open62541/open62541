#include "ua_services.h"
#include "ua_statuscodes.h"

#include "ua_namespace_0.h"
#include "ua_util.h"
#include "ua_namespace_manager.h"

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_Int32 *numberOfFoundIndices;
    UA_UInt16 *associatedIndices;
    UA_UInt32 differentNamespaceIndexCount = 0;
	UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->nodesToBrowseSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    if(UA_Array_new((void **)&(response->results), request->nodesToBrowseSize, &UA_[UA_BROWSERESULT])
       != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = request->nodesToBrowseSize;
    if(UA_Array_new((void **)&numberOfFoundIndices,request->nodesToBrowseSize,&UA_[UA_UINT32]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }

    if(UA_Array_new((void **)&associatedIndices,request->nodesToBrowseSize,&UA_[UA_UINT16]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }

    BUILD_INDEX_ARRAYS(request->nodesToBrowseSize,request->nodesToBrowse,nodeId,differentNamespaceIndexCount,associatedIndices,numberOfFoundIndices);

	UA_UInt32 *browseDescriptionIndices;
    if(UA_Array_new((void **)&browseDescriptionIndices,request->nodesToBrowseSize,&UA_[UA_UINT32]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }

    for(UA_UInt32 i = 0; i < differentNamespaceIndexCount; i++){
    	UA_Namespace *tmpNamespace;
    	UA_NamespaceManager_getNamespace(server->namespaceManager,associatedIndices[i],&tmpNamespace);
    	if(tmpNamespace != UA_NULL){

    	    //build up index array for each read operation onto a different namespace
    	    UA_UInt32 n = 0;
    	    for(UA_Int32 j = 0; j < request->nodesToBrowseSize; j++){
    	    	if(request->nodesToBrowse[j].nodeId.namespaceIndex == associatedIndices[i]){
    	    		browseDescriptionIndices[n] = j;
					n++;
    	    	}
    	    }
    	    //call read for every namespace
    		tmpNamespace->nodeStore->browseNodes(&request->requestHeader,
    				request->nodesToBrowse,
    				browseDescriptionIndices,
    				numberOfFoundIndices[i],
    				request->requestedMaxReferencesPerNode,
    				response->results,
    				response->diagnosticInfos);
    	}
    }
    UA_free(browseDescriptionIndices);
    UA_free(numberOfFoundIndices);
    UA_free(associatedIndices);
}


void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->browsePathsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    // Allocate space for a correct answer
    response->resultsSize = request->browsePathsSize;
    // _init of the elements is done in Array_new
    if(UA_Array_new((void **)&response->results, request->browsePathsSize, &UA_[UA_BROWSEPATHRESULT])
       != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    for(UA_Int32 i = 0;i < request->browsePathsSize;i++)
        response->results[i].statusCode = UA_STATUSCODE_BADNOMATCH; //FIXME: implement
}
