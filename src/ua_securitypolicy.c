#include "ua_plugin_securitypolicy.h"
#include "ua_types_generated_handling.h"


UA_CertificateList *UA_CertificateList_new(const UA_ByteString *certificate) {
    UA_CertificateList *ret = (UA_CertificateList *)UA_calloc(1, sizeof(UA_CertificateList));
    if(ret == NULL)
        return NULL;

    ret->certificate = UA_ByteString_new();
    if(ret->certificate == NULL) {
        UA_free(ret);
        return NULL;
    }
    if(UA_ByteString_copy(certificate, ret->certificate) != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(ret->certificate);
        UA_free(ret);
        return NULL;
    }

    return ret;
}

UA_StatusCode UA_CertificateList_prepend(UA_CertificateList **list, const UA_ByteString *certificate) {
    UA_CertificateList *ret = UA_CertificateList_new(certificate);
    if(ret == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ret->next = *list;
    *list = ret;

    return UA_STATUSCODE_GOOD;
}

void UA_CertificateList_delete(UA_CertificateList *list) {
    UA_CertificateList *cert = list;
    
    while(cert != NULL) {
        UA_CertificateList *next = cert->next;

        UA_ByteString_delete(cert->certificate);
        UA_free(cert);
        
        cert = next;
    }
}