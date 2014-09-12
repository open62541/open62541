#include "ua_services.h"

UA_Int32 Service_GetEndpoints(UA_Server                    *server,
                              const UA_GetEndpointsRequest *request,
                              UA_GetEndpointsResponse      *response) {
    UA_GetEndpointsResponse_init(response);
    response->endpointsSize = 1;
    UA_Array_new((void **)&response->endpoints, response->endpointsSize, &UA_.types[UA_ENDPOINTDESCRIPTION]);

    // security policy
    response->endpoints[0].securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",
                          &response->endpoints[0].securityPolicyUri);
    UA_String_copycstring("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary",
                          &response->endpoints[0].transportProfileUri);

    // usertoken policy
    response->endpoints[0].userIdentityTokensSize = 1;
    UA_Array_new((void **)&response->endpoints[0].userIdentityTokens,
                 response->endpoints[0].userIdentityTokensSize, &UA_.types[UA_USERTOKENPOLICY]);
    UA_UserTokenPolicy *token = &response->endpoints[0].userIdentityTokens[0];
    UA_UserTokenPolicy_init(token);
    UA_String_copycstring("my-anonymous-policy", &token->policyId); // defined per server
    token->tokenType         = UA_USERTOKENTYPE_ANONYMOUS;

    // server description
    UA_String_copy(&request->endpointUrl, &response->endpoints[0].endpointUrl);
    /* The standard says "the HostName specified in the Server Certificate is the
       same as the HostName contained in the endpointUrl provided in the
       EndpointDescription */
    UA_String_copy(&server->serverCertificate, &response->endpoints[0].serverCertificate);
    UA_String_copycstring("http://open62541.info/product/release", &(response->endpoints[0].server.productUri));
    // FIXME: This information should be provided by the application, preferably in the address space
    UA_String_copycstring("http://open62541.info/applications/4711",
                          &(response->endpoints[0].server.applicationUri));
    UA_LocalizedText_copycstring("The open62541 application", &(response->endpoints[0].server.applicationName));
    // FIXME: This should be a feature of the application and an enum
    response->endpoints[0].server.applicationType = UA_APPLICATIONTYPE_SERVER;

	return UA_SUCCESS;
}
