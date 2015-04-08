#ifdef NOT_AMALGATED
	#include "ua_types.h"
	#include "ua_client.h"
	#include "ua_nodeids.h"
#else
	#include "open62541.h"
#endif

#include <stdio.h>
#include "networklayer_tcp.h"

int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new();
	UA_ClientNetworkLayer nl = ClientNetworkLayerTCP_new(UA_ConnectionConfig_standard);
	UA_StatusCode retval = UA_Client_connect(client, UA_ConnectionConfig_standard, nl,
			"opc.tcp://localhost:16664");
	if(retval != UA_STATUSCODE_GOOD) {
		UA_Client_delete(client);
		return retval;
	}

	// Browse some objects
	UA_BrowseDescription bd;
	UA_BrowseDescription_init(&bd);
	bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	bd.resultMask = UA_BROWSERESULTMASK_ALL;	// everything

	UA_BrowseRequest breq;
	UA_BrowseRequest_init(&breq);
	breq.requestedMaxReferencesPerNode = 0;
	breq.nodesToBrowse = &bd;
	breq.nodesToBrowseSize = 1;

	UA_BrowseResponse bresp = UA_Client_browse(client, &breq);
	printf("result code: %d, results: %d, diagnostics: %d\n", bresp.responseHeader.serviceResult, bresp.resultsSize, bresp.diagnosticInfosSize);
	puts("id, namespace, browse name, display name");
	if (bresp.resultsSize > 0) for (int i = 0; i < bresp.resultsSize; ++i) {
		if (bresp.results[i].referencesSize > 0) for (int j = 0; j < bresp.results[i].referencesSize; ++j) {
			UA_ReferenceDescription *ref = &(bresp.results[i].references[j]);
			if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC){
				printf("%d, %d, %.*s, %.*s\n", ref->nodeId.nodeId.identifier.numeric, ref->browseName.namespaceIndex, ref->browseName.name.length, ref->browseName.name.data, ref->displayName.text.length, ref->displayName.text.data);
			}else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING){
				printf("%.*s, %d, %.*s, %.*s\n", ref->nodeId.nodeId.identifier.string.length, ref->nodeId.nodeId.identifier.string.data, ref->browseName.namespaceIndex, ref->browseName.name.length, ref->browseName.name.data, ref->displayName.text.length, ref->displayName.text.data);
			}else{
				printf("--, %d, %.*s, %.*s\n", ref->browseName.namespaceIndex, ref->browseName.name.length, ref->browseName.name.data, ref->displayName.text.length, ref->displayName.text.data);
			}
		}
	}
	UA_BrowseResponse_deleteMembers(&bresp);

	// Read a node
	UA_ReadRequest req;
	UA_ReadRequest_init(&req);
	req.nodesToRead = UA_ReadValueId_new();
	req.nodesToReadSize = 1;
	req.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer"); /* assume this node exists */
	req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

	UA_ReadResponse resp = UA_Client_read(client, &req);
	if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
	   resp.resultsSize > 0 && resp.results[0].hasValue &&
	   resp.results[0].value.data /* an empty array returns a null-ptr */ &&
	   resp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32])
		printf("the answer is: %i\n", *(UA_Int32*)resp.results[0].value.data);

	UA_ReadRequest_deleteMembers(&req);
	UA_ReadResponse_deleteMembers(&resp);
	UA_Client_disconnect(client);
	UA_Client_delete(client);
	return UA_STATUSCODE_GOOD;
}

