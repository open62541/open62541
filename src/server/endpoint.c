//
// Created by mark on 10.05.22.
//

#include <open62541/endpoint.h>
#include <open62541/plugin/securitypolicy.h>
#include "ua_util_internal.h"

typedef struct EndpointContext {
    UA_EndpointDescription endpointDescription;
} EndpointContext;

UA_StatusCode
UA_Endpoint_init(UA_Endpoint *endpoint,
                 const UA_String *endpointUrl,
                 UA_PKIStore *pkiStore,
                 UA_SecurityPolicy *securityPolicy,
                 UA_Boolean allowNone,
                 UA_Boolean allowSign,
                 UA_Boolean allowSignAndEncrypt,
                 UA_ApplicationDescription applicationDescription,
                 UA_UserTokenPolicy *userTokenPolicies,
                 size_t userTokenPoliciesSize) {
    if(endpoint == NULL || endpointUrl == NULL || pkiStore == NULL || securityPolicy == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    endpoint->pkiStore = pkiStore;
    endpoint->securityPolicy = securityPolicy;
    endpoint->allowNone = allowNone;
    endpoint->allowSign = allowSign;
    endpoint->allowSignAndEncrypt = allowSignAndEncrypt;
    UA_String_copy(endpointUrl, &endpoint->endpointUrl);
    endpoint->context = UA_malloc(sizeof(EndpointContext));
    if(endpoint->context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    EndpointContext *context = (EndpointContext *)endpoint->context;
    UA_EndpointDescription *endpointDescription = &context->endpointDescription;
    UA_EndpointDescription_init(endpointDescription);

    endpointDescription->securityMode = UA_MESSAGESECURITYMODE_INVALID;
    UA_String_copy(&securityPolicy->policyUri, &endpointDescription->securityPolicyUri);
    endpointDescription->transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    /* Add security level value for the corresponding message security mode */
    endpointDescription->securityLevel = (UA_Byte)UA_MESSAGESECURITYMODE_INVALID;

    /* Enable all login mechanisms from the access control plugin  */
    UA_StatusCode retval = UA_Array_copy(userTokenPolicies,
                                         userTokenPoliciesSize,
                                         (void **)&endpointDescription->userIdentityTokens,
                                         &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_String_clear(&endpointDescription->securityPolicyUri);
        UA_String_clear(&endpointDescription->transportProfileUri);
        return retval;
    }
    endpointDescription->userIdentityTokensSize = userTokenPoliciesSize;

    return UA_ApplicationDescription_copy(&applicationDescription, &endpointDescription->server);
}

UA_StatusCode
UA_Endpoint_toEndpointDescription(const UA_Endpoint *endpoint,
                                  UA_EndpointDescription *endpointDescription,
                                  UA_MessageSecurityMode securityMode) {
    EndpointContext *context = (EndpointContext *)endpoint->context;
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
    UA_StatusCode retval = UA_EndpointDescription_copy(&context->endpointDescription, endpointDescription);
    UA_CHECK_STATUS(retval, return retval);
    endpointDescription->securityMode = securityMode;
    endpointDescription->securityLevel = (UA_Byte)securityMode;
    retval = UA_String_copy(&endpoint->endpointUrl, &endpointDescription->endpointUrl);
    UA_CHECK_STATUS(retval, return retval);
    retval = endpoint->securityPolicy->getLocalCertificate(endpoint->securityPolicy,
                                                           endpoint->pkiStore,
                                                           &endpointDescription->serverCertificate);
    return retval;
}

void
UA_Endpoint_clear(UA_Endpoint *endpoint) {
    EndpointContext *context = (EndpointContext *)endpoint->context;
    UA_String_clear(&endpoint->endpointUrl);
    UA_EndpointDescription_clear(&context->endpointDescription);
    UA_free(context);
}

UA_StatusCode
UA_Endpoint_updateApplicationDescription(UA_Endpoint *endpoint,
                                         const UA_ApplicationDescription *applicationDescription) {
    EndpointContext *context = (EndpointContext *)endpoint->context;
    UA_ApplicationDescription_clear(&context->endpointDescription.server);
    return UA_ApplicationDescription_copy(applicationDescription, &context->endpointDescription.server);
}
