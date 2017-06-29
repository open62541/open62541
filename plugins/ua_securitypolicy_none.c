/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include "ua_securitypolicy_none.h"
#include "ua_types.h"
#include "ua_types_generated_handling.h"

#define UA_STRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}
#define UA_STRING_STATIC_NULL {0, NULL}

 /////////////////////////////////
 // Asymmetric module functions //
 /////////////////////////////////

static UA_StatusCode
asym_verify_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    const UA_ByteString *const message,
                    const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                  const void *const channelContext,
                  const UA_ByteString *const message,
                  UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                   const void *const channelContext) {
    return 0;
}

static size_t
asym_getRemoteSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                    const void *const channelContext) {
    return 0;
}

static UA_StatusCode
asym_encrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const channelContext,
                     UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const channelContext,
                     UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_makeThumbprint_sp_none(const UA_SecurityPolicy *const securityPolicy,
                            const UA_ByteString *const certificate,
                            UA_ByteString *const thumbprint) {
    if(certificate == NULL || thumbprint == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/////////////////////////////////////
// End asymmetric module functions //
/////////////////////////////////////

////////////////////////////////
// Symmetric module functions //
////////////////////////////////

static UA_StatusCode
sym_verify_sp_none(const UA_SecurityPolicy *const securityPolicy,
                   const void *const channelContext,
                   const UA_ByteString *const message,
                   const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                 const void *const channelContext,
                 const UA_ByteString *const message,
                 UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getLocalSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                  const void *const channelContext) {
    return 0;
}

static size_t
sym_getRemoteSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                   const void *const channelContext) {
    return 0;
}

static UA_StatusCode
sym_encrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_decrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_generateKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                        const UA_ByteString *const secret,
                        const UA_ByteString *const seed,
                        UA_ByteString *const out) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_generateNonce_sp_none(const UA_SecurityPolicy *const securityPolicy,
                          const void *const policyContext,
                          UA_ByteString *const out) {
    out->length = securityPolicy->symmetricModule.encryptingKeyLength;
    out->data[0] = 'a';

    return UA_STATUSCODE_GOOD;
}

////////////////////////////////////
// End symmetric module functions //
////////////////////////////////////

///////////////////////////////
// Security policy functions //
///////////////////////////////
static UA_StatusCode
verifyCertificate_sp_none(const UA_SecurityPolicy *const securityPolicy,
                          const void *const policyContext,
                          const void *const channelContext) {

    if(securityPolicy == NULL || policyContext == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

///////////////////////////////////
// End security policy functions //
///////////////////////////////////

/////////////////////////////
// PolicyContext functions //
/////////////////////////////

// this is not really needed in security policy none because no context is required
// it is there to serve as a small example for policies that need context per policy
typedef struct {
    const UA_SecurityPolicy *securityPolicy;
    UA_ByteString localCert;
} UA_SP_NONE_EndpointContextData;

static UA_StatusCode
policyContext_setLocalCertificate_sp_none(const UA_ByteString *const certificate,
                                            void *policyContext) {
    if(certificate == NULL || policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_SP_NONE_EndpointContextData *const data = (UA_SP_NONE_EndpointContextData*)policyContext;

    return UA_ByteString_copy(certificate, &data->localCert);
}

static UA_StatusCode
policyContext_newContext_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                   const UA_Policy_SecurityContext_RequiredInitData *initData,
                                   const void *const optInitData,
                                   void **const pp_contextData) {
    if(securityPolicy == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    *pp_contextData = UA_malloc(sizeof(UA_SP_NONE_EndpointContextData));

    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // Initialize the PolicyContext data to sensible values
    UA_SP_NONE_EndpointContextData *const data = (UA_SP_NONE_EndpointContextData*)*pp_contextData;

    data->securityPolicy = securityPolicy;

    UA_ByteString_init(&data->localCert);

    if (initData != NULL)
        policyContext_setLocalCertificate_sp_none(&initData->localCertificate, *pp_contextData);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Initialized PolicyContext for sp_none");

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
policyContext_deleteContext_sp_none(void *const securityContext) {
    if(securityContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // delete all allocated members in the data block
    UA_SP_NONE_EndpointContextData* data = (UA_SP_NONE_EndpointContextData*)securityContext;

    UA_ByteString_deleteMembers(&data->localCert);


    UA_LOG_DEBUG(data->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of PolicyContext for sp_none");

    UA_free(securityContext);

    return UA_STATUSCODE_GOOD;
}

static const UA_ByteString*
policyContext_getLocalCertificate_sp_none(const void *const policyContext) {
    if(policyContext == NULL)
        return NULL;

    const UA_SP_NONE_EndpointContextData *const data = (const UA_SP_NONE_EndpointContextData*)policyContext;

    return &data->localCert;
}

static UA_StatusCode
policyContext_compareCertificateThumbprint_sp_none(const void *const policyContext,
                                                     const UA_ByteString *const certificateThumbprint) {
    if(policyContext == NULL || certificateThumbprint == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // The certificatethumbprint doesn't exist if the security policy is none
    if(certificateThumbprint->length != 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

/////////////////////////////////
// End PolicyContext functions //
/////////////////////////////////

//////////////////////////////
// ChannelContext functions //
//////////////////////////////

// this is not really needed in security policy none because no context is required
// it is there to serve as a small example for policies that need context per channel
typedef struct {
    const UA_SecurityPolicy *securityPolicy;
    const void * policyContext;
    int callCounter;
} UA_SP_NONE_ChannelContextData;

static UA_StatusCode
channelContext_newContext_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                  const void *const policyContext,
                                  const UA_ByteString *const remoteCertificate,
                                  void **const pp_channelContext) {
    if(securityPolicy == NULL || pp_channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    *pp_channelContext = UA_malloc(sizeof(UA_SP_NONE_ChannelContextData));
    if(*pp_channelContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // Initialize the channelcontext data here to sensible values
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)*pp_channelContext;

    data->callCounter = 0;
    data->securityPolicy = securityPolicy;
    data->policyContext = policyContext;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode channelContext_deleteContext_sp_none(void *const channelContext) {
    if(channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // Delete the member variables that eventually were allocated in the init method
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    UA_LOG_DEBUG(data->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Call counter was %i before deletion.", data->callCounter);

    data->callCounter = 0;
    data->securityPolicy = NULL;
    data->policyContext = NULL;

    UA_free(channelContext);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_none(void *const channelContext,
                                                const UA_ByteString *const key) {
    if(channelContext == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalEncryptingKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_none(void *const channelContext,
                                             const UA_ByteString *const key) {
    if(channelContext == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalSigningKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_none(void *const channelContext,
                                     const UA_ByteString *const iv) {
    if(channelContext == NULL || iv == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalIv_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_none(void *const channelContext,
                                                 const UA_ByteString *const key) {
    if(channelContext == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteEncryptingKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_none(void *const channelContext,
                                              const UA_ByteString *const key) {
    if(channelContext == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteSigningKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_none(void *const channelContext,
                                      const UA_ByteString *const iv) {
    if(channelContext == NULL || iv == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteIv_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_compareCertificate_sp_none(const void *const channelContext,
                                          const UA_ByteString *const certificate) {
    if(channelContext == NULL || certificate == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static size_t
channelContext_getRemoteAsymPlainTextBlockSize_sp_none(const void *const channelContext) {
    return 0;
}

static size_t
channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none(const void *const channelContext,
                                                                   const size_t maxEncryptionLength) {
    return 0;
}

//////////////////////////////////
// End ChannelContext functions //
//////////////////////////////////

UA_EXPORT UA_SecurityPolicy UA_SecurityPolicy_None = {
    /* The policy uri that identifies the implemented algorithms */
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None"), // .policyUri

    /* Asymmetric module */
    { // .asymmetricModule
        asym_makeThumbprint_sp_none, // .makeThumbprint

        0, // .minAsymmetricKeyLength
        0, // .maxAsymmetricKeyLength
        20, // .thumbprintLength

        { // .encryptingModule
            asym_encrypt_sp_none, // .encrypt
            asym_decrypt_sp_none // .decrypt
        },
    /* Asymmetric signing module */
    {
        asym_verify_sp_none, // .verify
        asym_sign_sp_none, // .sign
        asym_getLocalSignatureSize_sp_none, // .getLocalSignatureSize
        asym_getRemoteSignatureSize_sp_none, // .getRemoteSignatureSize
        UA_STRING_STATIC_NULL // .signatureAlgorithmUri
    }
},

/* Symmetric module */
{ // .symmetricModule
    sym_generateKey_sp_none, // .generateKey
    sym_generateNonce_sp_none, // .generateNonce 

    { // .encryptingModule
        sym_encrypt_sp_none, // .encrypt
        sym_decrypt_sp_none // .decrypt
    },

    /* Symmetric signing module */
    { // .signingModule
        sym_verify_sp_none, // .verify
        sym_sign_sp_none, // .sign
        sym_getLocalSignatureSize_sp_none, // .getLocalSignatureSize
        sym_getRemoteSignatureSize_sp_none, // .getRemoteSignatureSize
        UA_STRING_STATIC_NULL // .signatureAlgorithmUri
    },

    0, // .signingKeyLength
    1, // .encryptingKeyLength
    0 // .encryptingBlockSize
},

{ // .context
    policyContext_newContext_sp_none, // .init
    policyContext_deleteContext_sp_none, // .deleteMembers
    policyContext_getLocalCertificate_sp_none,
    policyContext_compareCertificateThumbprint_sp_none
},

{ // .channelContext
    channelContext_newContext_sp_none,  // .new
    channelContext_deleteContext_sp_none, // .delete

    channelContext_setLocalSymEncryptingKey_sp_none, // .setLocalSymEncryptingKey
    channelContext_setLocalSymSigningKey_sp_none, // .setLocalSymSigningKey
    channelContext_setLocalSymIv_sp_none, // .setLocalSymIv

    channelContext_setRemoteSymEncryptingKey_sp_none, // .setRemoteSymEncryptingKey
    channelContext_setRemoteSymSigningKey_sp_none, // .setRemoteSymSigningKey
    channelContext_setRemoteSymIv_sp_none, // .setRemoteSymIv

    channelContext_compareCertificate_sp_none, // .parseRemoteCertificate

    channelContext_getRemoteAsymPlainTextBlockSize_sp_none, // .getRemoteAsymPlainTextBlockSize
    channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none // .getRemoteAsymEncryptionBufferLengthOverhead
},

NULL // .logger
};
