#include "ua_services.h"
UA_Int32 Service_GetEndpoints(SL_secureChannel channel, const UA_GetEndpointsRequest* request, UA_GetEndpointsResponse *response) {
#ifdef DEBUG
	UA_String_printx("endpointUrl=", &request->endpointUrl);
#endif

	response->endpointsSize = 1;
	UA_Array_new((void**) &response->endpoints,response->endpointsSize,&UA_.types[UA_ENDPOINTDESCRIPTION]);

	//Security issues:
	//The policy should be 'http://opcfoundation.org/UA/SecurityPolicy#None'
	//FIXME String or ByteString
	UA_AsymmetricAlgorithmSecurityHeader *asymSettings;
	SL_Channel_getLocalAsymAlgSettings(channel, &asymSettings);
	UA_String_copy((UA_String *)&(asymSettings->securityPolicyUri), &response->endpoints[0].securityPolicyUri);
	//FIXME hard-coded code
	response->endpoints[0].securityMode = UA_MESSAGESECURITYMODE_NONE;
	UA_String_copycstring("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary", &response->endpoints[0].transportProfileUri);

	response->endpoints[0].userIdentityTokensSize = 1;
	UA_Array_new((void**) &response->endpoints[0].userIdentityTokens, response->endpoints[0].userIdentityTokensSize,
				 &UA_.types[UA_USERTOKENPOLICY]);
	UA_UserTokenPolicy *token = &response->endpoints[0].userIdentityTokens[0];
	UA_String_copycstring("my-anonymous-policy", &token->policyId); // defined per server
	token->tokenType = UA_USERTOKENTYPE_ANONYMOUS;
	token->issuerEndpointUrl = (UA_String) {-1, UA_NULL};
	token->issuedTokenType = (UA_String) {-1, UA_NULL};
	token->securityPolicyUri = (UA_String) {-1, UA_NULL};

	UA_String_copy(&request->endpointUrl,&response->endpoints[0].endpointUrl);
	UA_String_copycstring("http://open62541.info/product/release",&(response->endpoints[0].server.productUri));
	// FIXME: This information should be provided by the application, preferably in the address space
	UA_String_copycstring("http://open62541.info/applications/4711",&(response->endpoints[0].server.applicationUri));
	UA_LocalizedText_copycstring("The open62541 application",&(response->endpoints[0].server.applicationName));
	// FIXME: This should be a feature of the application and an enum
	response->endpoints[0].server.applicationType = UA_APPLICATIONTYPE_SERVER;
	// all the other strings are empty by initialization
	
	return UA_SUCCESS;
}
