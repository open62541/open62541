#include "ua_services.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"
#include "ua_server_internal.h"
#include "ua_services_internal.h"
#include "ua_namespace_manager.h"
#include "ua_session.h"
#include "ua_util.h"

void Service_AddNodes(UA_Server *server, UA_Session *session,
		const UA_AddNodesRequest *request, UA_AddNodesResponse *response) {
	UA_assert(server != UA_NULL && session != UA_NULL);

	if (request->nodesToAddSize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		return;
	}

	if (UA_Array_new((void **) &response->results, request->nodesToAddSize,
			&UA_TYPES[UA_ADDNODESRESULT]) != UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	response->resultsSize = request->nodesToAddSize;
	if (UA_Array_new((void **) &response->diagnosticInfos,
			request->nodesToAddSize, &UA_TYPES[UA_DIAGNOSTICINFO])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	response->diagnosticInfosSize = request->nodesToAddSize;



	UA_Int32 *numberOfFoundIndices;
	UA_UInt16 *associatedIndices;
	UA_UInt32 differentNamespaceIndexCount = 0;
	if (UA_Array_new((void **) &numberOfFoundIndices,
			request->nodesToAddSize, &UA_TYPES[UA_UINT32])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}

	if (UA_Array_new((void **) &associatedIndices, request->nodesToAddSize,
			&UA_TYPES[UA_UINT16]) != UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	// find out count of different namespace indices
	BUILD_INDEX_ARRAYS(request->nodesToAddSize,request->nodesToAdd,requestedNewNodeId.nodeId,differentNamespaceIndexCount,associatedIndices,numberOfFoundIndices);

	UA_UInt32 *addNodesIndices;
	if (UA_Array_new((void **) &addNodesIndices,
			request->nodesToAddSize, &UA_TYPES[UA_UINT32])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}

	for (UA_UInt32 i = 0; i < differentNamespaceIndexCount; i++) {
		UA_Namespace *tmpNamespace;
		UA_NamespaceManager_getNamespace(server->namespaceManager,
				associatedIndices[i], &tmpNamespace);
		if (tmpNamespace != UA_NULL) {
			//build up index array for each read operation onto a different namespace
			UA_UInt32 n = 0;
			for (UA_Int32 j = 0; j < request->nodesToAddSize; j++) {
				if (request->nodesToAdd[j].requestedNewNodeId.nodeId.namespaceIndex
						== associatedIndices[i]) {
					addNodesIndices[n] = j;
					n++;
				}
			}
			//call read for every namespace
			tmpNamespace->nodeStore->addNodes(&request->requestHeader,request->nodesToAdd,
					addNodesIndices, numberOfFoundIndices[i],
					response->results, response->diagnosticInfos);
		}
	}
	UA_free(addNodesIndices);
	UA_free(numberOfFoundIndices);
	UA_free(associatedIndices);
}



void Service_AddReferences(UA_Server *server, UA_Session *session,
		const UA_AddReferencesRequest *request,
		UA_AddReferencesResponse *response) {

	if (request->referencesToAddSize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		return;
	}

	if (UA_Array_new((void **) &response->results, request->referencesToAddSize,
			&UA_TYPES[UA_STATUSCODE]) != UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	response->resultsSize = request->referencesToAddSize;

	if (UA_Array_new((void **) &response->diagnosticInfos,
			request->referencesToAddSize, &UA_TYPES[UA_DIAGNOSTICINFO])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	response->diagnosticInfosSize = request->referencesToAddSize;
	UA_Int32 *numberOfFoundIndices;
	UA_UInt16 *associatedIndices;
	UA_UInt32 differentNamespaceIndexCount = 0;
	if (UA_Array_new((void **) &numberOfFoundIndices,
			request->referencesToAddSize, &UA_TYPES[UA_UINT32])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}

	if (UA_Array_new((void **) &associatedIndices, request->referencesToAddSize,
			&UA_TYPES[UA_UINT16]) != UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	// find out count of different namespace indices


	BUILD_INDEX_ARRAYS(request->referencesToAddSize, request->referencesToAdd, sourceNodeId,differentNamespaceIndexCount, associatedIndices, numberOfFoundIndices);

	UA_UInt32 *readValueIdIndices;
	if (UA_Array_new((void **) &readValueIdIndices,
			request->referencesToAddSize, &UA_TYPES[UA_UINT32])
			!= UA_STATUSCODE_GOOD) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}

	for (UA_UInt32 i = 0; i < differentNamespaceIndexCount; i++) {
		UA_Namespace *tmpNamespace;
		UA_NamespaceManager_getNamespace(server->namespaceManager,
				associatedIndices[i], &tmpNamespace);
		if (tmpNamespace != UA_NULL) {
			//build up index array for each read operation onto a different namespace
			UA_UInt32 n = 0;
			for (UA_Int32 j = 0; j < request->referencesToAddSize; j++) {
				if (request->referencesToAdd[j].sourceNodeId.namespaceIndex
						== associatedIndices[i]) {
					readValueIdIndices[n] = j;
					n++;
				}
			}
			//call read for every namespace
			tmpNamespace->nodeStore->addReferences(&request->requestHeader,request->referencesToAdd,
					readValueIdIndices, numberOfFoundIndices[i],
					response->results, response->diagnosticInfos);

			//	response->results[i] = service_read_node(server, &request->nodesToRead[i]);
		}
	}
	UA_free(readValueIdIndices);
	UA_free(numberOfFoundIndices);
	UA_free(associatedIndices);
	/*
	 for(UA_Int32 i = 0;i < response->resultsSize;i++){
	 response->results[i] = service_read_node(server, &request->nodesToRead[i]);
	 }
	 }
	 */


	//response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}
