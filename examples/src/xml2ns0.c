#include <fcntl.h> // open, O_RDONLY
#include "ua_types.h"
//#include "ua_types_generated.h"
#include "ua_namespace.h"
#include "ua_xml.h"
#include "ua_namespace_xml.h"
#include "ua_types_encoding_xml.h"
#include "util/ua_util.h"

// FIXME: most of the following code should be generated as a template from the ns0-defining xml-files
/** @brief sam (abbr. for server_application_memory) is a flattened memory structure of the UAVariables in ns0 */
struct sam {
	UA_ServerStatusDataType serverStatus;
} sam;

char* productName = "xml2ns0";
char* productUri = "http://open62541.org/xml2ns0/";
char* manufacturerName = "open62541";
char* softwareVersion = "0.01";
char* buildNumber = "999-" __DATE__ "-001" ;

#define SAM_ASSIGN_CSTRING(src,dst) do { \
	dst.length = strlen(src)-1; \
	dst.data = (UA_Byte*) src; \
} while(0)

void sam_attach(Namespace *ns,UA_UInt32 ns0id,UA_Int32 type, void* p) {
	Namespace_Entry_Lock* lock;
	UA_NodeId nodeid;
	nodeid.namespace = ns->namespaceId;
	nodeid.identifier.numeric = ns0id;
	nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	const UA_Node* result;
	UA_Int32 retval;
	if ((retval = Namespace_get(ns,&nodeid,&result,&lock)) == UA_SUCCESS) {
		if (result->nodeId.identifier.numeric == ns0id) {
			if (result->nodeClass == UA_NODECLASS_VARIABLE) {
				UA_VariableNode* variable = (UA_VariableNode*) result;
				if (variable->dataType.identifier.numeric == UA_.types[type].typeId.identifier.numeric) {
					variable->value.arrayLength = 1;
					variable->value.data = p;
				} else {
					UA_alloc((void**)&variable->value.data, sizeof(UA_ExtensionObject));
					UA_ExtensionObject* eo = (UA_ExtensionObject*) variable->value.data;
					eo->typeId = variable->dataType;
					eo->encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
					// FIXME: This code is valid for ns0 and numeric identifiers only
					eo->body.length = UA_.types[UA_ns0ToVTableIndex(&variable->dataType)].memSize;
					eo->body.data = p;
				}
			}
		}
	}
}
	/* 	} else { */
	/* 		perror("Namespace_getWritable returned wrong node class"); */
	/* 	} */
	/* } else { */
	/* 	printf("retrieving node={i=%d} returned node={i=%d}\n", ns0id, result->nodeId.identifier.numeric); */
	/* } */
	/*        Namespace_Entry_Lock_release(lock); */
	/* 	} else { */
	/* 	printf("retrieving node={i=%d} returned error code %d\n",ns0id,retval); */
	/* } */

void sam_init(Namespace* ns) {
	// Initialize the strings
	SAM_ASSIGN_CSTRING(productName,sam.serverStatus.buildInfo.productName);
	SAM_ASSIGN_CSTRING(productUri,sam.serverStatus.buildInfo.productUri);
	SAM_ASSIGN_CSTRING(manufacturerName,sam.serverStatus.buildInfo.manufacturerName);
	SAM_ASSIGN_CSTRING(softwareVersion,sam.serverStatus.buildInfo.softwareVersion);
	SAM_ASSIGN_CSTRING(buildNumber,sam.serverStatus.buildInfo.buildNumber);

	// Attach server application memory to ns0
	sam_attach(ns,2256,UA_SERVERSTATUSDATATYPE,&sam.serverStatus); // this is the head of server status!
	sam.serverStatus.startTime = sam.serverStatus.currentTime = UA_DateTime_now();
	sam_attach(ns,2257,UA_DATETIME, &sam.serverStatus.startTime);
	sam_attach(ns,2258,UA_DATETIME, &sam.serverStatus.currentTime);
	sam_attach(ns,2259,UA_SERVERSTATE, &sam.serverStatus.state);
	sam_attach(ns,2260,UA_BUILDINFO, &sam.serverStatus.buildInfo); // start of build Info
	sam_attach(ns,2261,UA_STRING, &sam.serverStatus.buildInfo.productName);
	sam_attach(ns,2262,UA_STRING, &sam.serverStatus.buildInfo.productUri);
	sam_attach(ns,2263,UA_STRING, &sam.serverStatus.buildInfo.manufacturerName);
	sam_attach(ns,2264,UA_STRING, &sam.serverStatus.buildInfo.softwareVersion);
	sam_attach(ns,2265,UA_STRING, &sam.serverStatus.buildInfo.buildNumber);
	sam_attach(ns,2266,UA_DATETIME, &sam.serverStatus.buildInfo.buildDate);
	sam_attach(ns,2992,UA_UINT32, &sam.serverStatus.secondsTillShutdown);
	sam_attach(ns,2993,UA_LOCALIZEDTEXT,&sam.serverStatus.shutdownReason);
}

UA_Int32 Namespace_getNumberOfComponents(Namespace const * ns, UA_NodeId const * id, UA_Int32* number) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;
	if (node == UA_NULL)
		return UA_ERR_INVALID_VALUE;
	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i].referenceTypeId.identifier.numeric == 47 && node->references[i].isInverse != UA_TRUE) {
			n++;
		}
	}
	*number = n;
	return retval;
}

UA_Int32 Namespace_getComponent(Namespace const * ns, UA_NodeId const * id, UA_Int32 idx, UA_NodeId** result) {
	UA_Int32 retval = UA_SUCCESS;

	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;

	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i].referenceTypeId.identifier.numeric == 47 && node->references[i].isInverse != UA_TRUE) {
			n++;
			if (n == idx) {
				*result = &(node->references[i].targetId.nodeId);
				return retval;
			}
		}
	}
	return UA_ERR_INVALID_VALUE;
}


UA_Int32 UAX_NodeId_encodeBinaryByMetaData(Namespace const * ns, UA_NodeId const * id, UA_ByteString *dst, UA_UInt32* offset) {
	UA_Int32 i, retval = UA_SUCCESS;
	if (UA_NodeId_isBasicType(id)) {
		UA_Node const * result;
		Namespace_Entry_Lock* lock;
		if ((retval = Namespace_get(ns,id,&result,&lock)) == UA_SUCCESS)
			UA_Variant_encodeBinary(&((UA_VariableNode *) result)->value,dst,offset);
	} else {
		UA_Int32 nComp = 0;
		if ((retval = Namespace_getNumberOfComponents(ns,id,&nComp)) == UA_SUCCESS) {
			for (i=0; i < nComp; i++) {
				UA_NodeId* comp = UA_NULL;
				Namespace_getComponent(ns,id,i,&comp);
				UAX_NodeId_encodeBinaryByMetaData(ns,comp, dst, offset);
			}
		}
	}
	return retval;
}

UA_Int32 UAX_NodeId_encodeBinary(Namespace const * ns, UA_NodeId const * id, UA_ByteString *dst, UA_UInt32* offset) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Node const * node;
	Namespace_Entry_Lock* lock;

	if ((retval = Namespace_get(ns,id,&node,&lock)) == UA_SUCCESS) {
		if (node->nodeClass == UA_NODECLASS_VARIABLE) {
			retval = UA_Variant_encodeBinary(&((UA_VariableNode*) node)->value,dst,offset);
		}
		Namespace_Entry_Lock_release(lock);
	}
	return retval;
}

int main() {
	Namespace* ns;
	UA_Int32 retval;

	retval = Namespace_loadFromFile(&ns, 0, "ROOT", UA_NULL);

	sam_init(ns);
	// Namespace_iterate(ns, print_node);

	// encoding buffer
	char buf[1024];
	UA_ByteString buffer = { 1024, (UA_Byte*) buf };
	UA_UInt32 pos = 0;

	UA_NodeId nodeid;
	nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	nodeid.namespace = 0;
	nodeid.identifier.numeric = 2256; // ServerStatus

	UA_Int32 i=0;
	retval=UA_SUCCESS;
	UA_DateTime tStart = UA_DateTime_now();
	// encoding takes roundabout 10 µs on my virtual machine with -O0, so 1E5 takes a second
	for (i=0;i<1E5 && retval == UA_SUCCESS;i++) {
		pos = 0;
		sam.serverStatus.currentTime = UA_DateTime_now();
		retval |= UAX_NodeId_encodeBinary(ns,&nodeid,&buffer,&pos);
	}
	UA_DateTime tEnd = UA_DateTime_now();
	// tStart, tEnd count in 100 ns steps, so 10 steps = 1 µs
	UA_Double tDelta = ( tEnd - tStart ) / ( 10.0 * i );

	printf("encode server node %d times, retval=%d: time/enc=%f us, enc/s=%f\n",i, retval, tDelta, 1.0E6 / tDelta);
	DBG(buffer.length=pos);
	DBG(UA_ByteString_printx(", result=", &buffer));

	// Design alternative two : use meta data
	// pos = 0;
	// buffer.length = 1024;
	// UAX_NodeId_encodeBinary(n.ns,&nodeid,&pos,&buffer);
	// buffer.length = pos;
	// UA_ByteString_printx("namespace based encoder result=", &buffer);

	return 0;
}
