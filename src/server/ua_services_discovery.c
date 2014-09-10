#include "ua_services.h"
#include "errno.h"

UA_Int32 Service_GetEndpoints(SL_Channel *channel,
		const UA_GetEndpointsRequest* request,
		UA_GetEndpointsResponse *response) {
#define RETURN free(certificate_base64_without_lb); fclose(fp); return

#ifdef DEBUG
	UA_String_printx("endpointUrl=", &request->endpointUrl);
#endif

	response->endpointsSize = 1;
	UA_Array_new((void**) &response->endpoints, response->endpointsSize,
			&UA_.types[UA_ENDPOINTDESCRIPTION]);

	//Security issues:
	//The policy should be 'http://opcfoundation.org/UA/SecurityPolicy#None'
	//FIXME String or ByteString
	UA_AsymmetricAlgorithmSecurityHeader *asymSettings;
	SL_Channel_getLocalAsymAlgSettings(channel, &asymSettings);
	UA_String_copy((UA_String *) &(asymSettings->securityPolicyUri),
			&response->endpoints[0].securityPolicyUri);
	//FIXME hard-coded code
	response->endpoints[0].securityMode = UA_MESSAGESECURITYMODE_NONE;
	UA_String_copycstring(
			"http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary",
			&response->endpoints[0].transportProfileUri);

	response->endpoints[0].userIdentityTokensSize = 1;
	UA_Array_new((void**) &response->endpoints[0].userIdentityTokens,
			response->endpoints[0].userIdentityTokensSize,
			&UA_.types[UA_USERTOKENPOLICY]);
	UA_UserTokenPolicy *token = &response->endpoints[0].userIdentityTokens[0];
	UA_String_copycstring("my-anonymous-policy", &token->policyId); // defined per server
	token->tokenType = UA_USERTOKENTYPE_ANONYMOUS;
	token->issuerEndpointUrl = (UA_String ) { -1, UA_NULL };
	token->issuedTokenType = (UA_String ) { -1, UA_NULL };
	token->securityPolicyUri = (UA_String ) { -1, UA_NULL };

	UA_String_copy(&request->endpointUrl, &response->endpoints[0].endpointUrl);
	UA_String_copycstring("http://open62541.org/product/release",
			&(response->endpoints[0].server.productUri));
	// FIXME: This information should be provided by the application, preferably in the address space
	UA_String_copycstring("urn:localhost:open6251:open62541Server",
			&(response->endpoints[0].server.applicationUri));
	UA_LocalizedText_copycstring("The open62541 application",
			&(response->endpoints[0].server.applicationName));
	// FIXME: This should be a feature of the application and an enum
	response->endpoints[0].server.applicationType = UA_APPLICATIONTYPE_SERVER;
	// all the other strings are empty by initialization

	FILE *fp = UA_NULL;
	UA_UInt32 certificate_size=0;
	UA_Byte* certificate= UA_NULL;
	//FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
	//FIXME: we need to read file only once
	fp=fopen("localhost.der", "rb");
	if (fp == NULL) {
			//printf("Opening the certificate file failed, errno = %d\n", errno);
	        //RETURN UA_ERROR;
	}else{
		fseek(fp, 0, SEEK_END);
		certificate_size = ftell(fp);

		UA_alloc((void**)&certificate, certificate_size*sizeof(UA_Byte));
		//read certificate without linebreaks
		fseek(fp, 0, SEEK_SET);
		fread(certificate, sizeof(UA_Byte), certificate_size, fp);

		UA_String certificate_binary;
		certificate_binary.length = certificate_size;
		certificate_binary.data = certificate;

		fclose(fp);
		//The standard says "the HostName specified in the Server Certificate is the same as the HostName contained in the
		//endpointUrl provided in the EndpointDescription;"

		UA_String_copy(&certificate_binary, &response->endpoints[0].serverCertificate);
		UA_String_deleteMembers(&certificate_binary);
	}

	return UA_SUCCESS;
}
