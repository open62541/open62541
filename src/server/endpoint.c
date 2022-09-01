//
// Created by mark on 10.05.22.
//

#include <open62541/endpoint.h>
#include <open62541/plugin/securitypolicy.h>
#include "ua_util_internal.h"

UA_StatusCode
UA_Endpoint_init(UA_Endpoint* endpoint) {
	if (endpoint == NULL) {
		return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	memset(endpoint, 0, sizeof(UA_Endpoint));
	return UA_STATUSCODE_GOOD;
}

void
UA_Endpoint_clear(
	UA_Endpoint* endpoint
) {
	if (endpoint == NULL) {
		return;
	}

    UA_String_clear(&endpoint->endpointUrl);
    if (endpoint->endpointDescription != NULL) {
    	UA_EndpointDescription_clear(endpoint->endpointDescription);
    	UA_free(endpoint->endpointDescription);
    	endpoint->endpointDescription = NULL;
    }
}

UA_StatusCode
UA_Endpoint_setValues(
	UA_Endpoint* endpoint,
    const UA_String* endpointUrl,
    UA_PKIStore* pkiStore,
    UA_SecurityPolicy* securityPolicy,
    UA_Boolean allowNone,
    UA_Boolean allowSign,
    UA_Boolean allowSignAndEncrypt,
    UA_ApplicationDescription applicationDescription,
    UA_UserTokenPolicy* userTokenPolicies,
    size_t userTokenPoliciesSize
) {
	/* Check parameter */
    if(endpoint == NULL || endpointUrl == NULL || pkiStore == NULL || securityPolicy == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Set values */
    endpoint->pkiStore = pkiStore;
    endpoint->securityPolicy = securityPolicy;
    endpoint->allowNone = allowNone;
    endpoint->allowSign = allowSign;
    endpoint->allowSignAndEncrypt = allowSignAndEncrypt;
    UA_String_copy(endpointUrl, &endpoint->endpointUrl);
    if (endpoint->endpointDescription != NULL) {
    	free(endpoint->endpointDescription);
    	endpoint->endpointDescription = NULL;
    }
    endpoint->endpointDescription = (UA_EndpointDescription*)UA_malloc(sizeof(UA_EndpointDescription));
    if (endpoint->endpointDescription == NULL) {
    	return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_EndpointDescription_init(endpoint->endpointDescription);
    endpoint->endpointDescription->securityMode = UA_MESSAGESECURITYMODE_INVALID;
    UA_String_copy(&securityPolicy->policyUri, &endpoint->endpointDescription->securityPolicyUri);
    endpoint->endpointDescription->transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    /* Add security level value for the corresponding message security mode */
    endpoint->endpointDescription->securityLevel = (UA_Byte)UA_MESSAGESECURITYMODE_INVALID;

    /* Enable all login mechanisms from the access control plugin  */
    UA_StatusCode retval = UA_Array_copy(userTokenPolicies,
                                         userTokenPoliciesSize,
                                         (void **)&endpoint->endpointDescription->userIdentityTokens,
                                         &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_String_clear(&endpoint->endpointDescription->securityPolicyUri);
        UA_String_clear(&endpoint->endpointDescription->transportProfileUri);
        UA_EndpointDescription_clear(endpoint->endpointDescription);
        return retval;
    }
    endpoint->endpointDescription->userIdentityTokensSize = userTokenPoliciesSize;

    return UA_ApplicationDescription_copy(&applicationDescription, &endpoint->endpointDescription->server);
}

UA_StatusCode
UA_Endpoint_toEndpointDescription(
	const UA_Endpoint* endpoint,
    UA_EndpointDescription* endpointDescription,
    UA_MessageSecurityMode securityMode
) {
    switch(securityMode) {
    case UA_MESSAGESECURITYMODE_NONE:
        if(!endpoint->allowNone) return UA_STATUSCODE_BADINTERNALERROR;
        break;
    case UA_MESSAGESECURITYMODE_SIGN:
        if(!endpoint->allowSign) return UA_STATUSCODE_BADINTERNALERROR;
        break;
    case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
        if(!endpoint->allowSignAndEncrypt) return UA_STATUSCODE_BADINTERNALERROR;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode retval = UA_EndpointDescription_copy(endpoint->endpointDescription, endpointDescription);
    UA_CHECK_STATUS(retval, return retval);
    endpointDescription->securityMode = securityMode;
    endpointDescription->securityLevel = (UA_Byte)securityMode;
    retval = UA_String_copy(&endpoint->endpointUrl, &endpointDescription->endpointUrl);
    UA_CHECK_STATUS(retval, return retval);

    UA_ByteString_clear(&endpointDescription->serverCertificate);
    retval = endpoint->securityPolicy->getLocalCertificate(endpoint->securityPolicy,
                                                           endpoint->pkiStore,
                                                           &endpointDescription->serverCertificate);
    return retval;
}

UA_StatusCode
UA_Endpoint_updateApplicationDescription(
	UA_Endpoint* endpoint,
    const UA_ApplicationDescription *applicationDescription
) {
    UA_ApplicationDescription_clear(&endpoint->endpointDescription->server);
    return UA_ApplicationDescription_copy(applicationDescription, &endpoint->endpointDescription->server);
}

bool
UA_Endpoint_matchesUrl(
	UA_Endpoint* endpoint,
	UA_String* url
)
{
	/* Check parameter */
	if (endpoint == NULL || url == NULL) {
		return false;
	}

	/* Get host part from url */
	UA_String urlHost = UA_STRING_NULL;
	UA_String urlPath = UA_STRING_NULL;
	u16 urlPort = 0;
	UA_parseEndpointUrl(url, &urlHost, &urlPort, &urlPath);

	/* Check localhost */
	if (memcmp((char*)urlHost.data, "localhost", (size_t)urlHost.length) == 0 ||
	    memcmp((char*)urlHost.data, "127.0.0.1", (size_t)urlHost.length) == 0) {

		/* Get hostname */
		char host[256];
		UA_gethostname(host, sizeof(host));
		UA_String hostname = UA_STRING(host);

		/* Get host part from endpoint url */
		UA_String endpointHost = UA_STRING_NULL;
		UA_String endpointPath = UA_STRING_NULL;
		u16 endpointPort = 0;
		UA_parseEndpointUrl(&endpoint->endpointUrl, &endpointHost, &endpointPort, &endpointPath);

		/* The hostname also contains the loopback address */
		if (UA_String_equal(&hostname, &endpointHost) &&
		    UA_String_equal(&urlPath, &endpointPath) &&
			urlPort == endpointPort) {
			return true;
		}
	}

	return UA_String_equal(url, &endpoint->endpointUrl);
}
