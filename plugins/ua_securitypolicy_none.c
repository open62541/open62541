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
                    const void *const context,
                    const UA_ByteString *const message,
                    const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                  const void *const context,
                  const UA_ByteString *const message,
                  UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_encrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const endpointContext,
                     const void *const channelContext,
                     UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const endpointContext,
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

static UA_UInt16
asym_calculatePadding_sp_none(const UA_SecurityPolicy *const securityPolicy,
                              const void *const channelContext,
                              const void *const endpointContext,
                              const size_t bytesToWrite,
                              UA_Byte *const paddingSize,
                              UA_Byte *const extraPaddingSize) {
    if(securityPolicy == NULL || paddingSize == NULL || extraPaddingSize == NULL)
        return 0;

    *paddingSize = 0;
    *extraPaddingSize = 0;
    return 0;
}

/////////////////////////////////////
// End asymmetric module functions //
/////////////////////////////////////

////////////////////////////////
// Symmetric module functions //
////////////////////////////////

static UA_StatusCode
sym_verify_sp_none(const UA_SecurityPolicy *const securityPolicy,
                   const void *const context,
                   const UA_ByteString *const message,
                   const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                 const void *const context,
                 const UA_ByteString *const message,
                 UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
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
                          const void *const endpointContext,
                          UA_ByteString *const out) {
    out->length = securityPolicy->symmetricModule.encryptingKeyLength;
    out->data[0] = 'a';

    return UA_STATUSCODE_GOOD;
}

static UA_UInt16
sym_calculatePadding_sp_none(const UA_SecurityPolicy *const securityPolicy,
                             const size_t bytesToWrite,
                             UA_Byte *const paddingSize,
                             UA_Byte *const extraPaddingSize) {
    if(securityPolicy == NULL || paddingSize == NULL || extraPaddingSize == NULL)
        return 0;

    *paddingSize = 0;
    *extraPaddingSize = 0;
    return 0;
}

////////////////////////////////////
// End symmetric module functions //
////////////////////////////////////

///////////////////////////////
// Security policy functions //
///////////////////////////////
static UA_StatusCode
verifyCertificate_sp_none(const UA_SecurityPolicy *const securityPolicy,
                          const void *const endpointContext,
                          const void *const channelContext) {

    if(securityPolicy == NULL || endpointContext == NULL || channelContext == NULL)
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
    UA_ByteString localCert;
} UA_SP_NONE_EndpointContextData;

static UA_StatusCode
endpointContext_new_sp_none(const UA_SecurityPolicy *const securityPolicy,
                            const void *const initData,
                            void **const pp_contextData) {
    if(securityPolicy == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    *pp_contextData = UA_malloc(sizeof(UA_SP_NONE_EndpointContextData));

    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // Initialize the PolicyContext data to sensible values
    UA_SP_NONE_EndpointContextData *const data = (UA_SP_NONE_EndpointContextData*)*pp_contextData;

    UA_ByteString_init(&data->localCert);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Initialized PolicyContext for sp_none");

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
endpointContext_delete_sp_none(const UA_SecurityPolicy *const securityPolicy,
                               void *const securityContext) {
    if(securityContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // delete all allocated members in the data block
    UA_SP_NONE_EndpointContextData* data = (UA_SP_NONE_EndpointContextData*)securityContext;

    UA_ByteString_deleteMembers(&data->localCert);

    UA_free(securityContext);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of PolicyContext for sp_none");

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
endpointContext_setLocalPrivateKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                            const UA_ByteString *const privateKey,
                                            void *const endpointContext) {
    if(securityPolicy == NULL || privateKey == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
endpointContext_setServerCertificate_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                             const UA_ByteString *const certificate,
                                             void *endpointContext) {
    if(securityPolicy == NULL || certificate == NULL || endpointContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_SP_NONE_EndpointContextData *const data = (UA_SP_NONE_EndpointContextData*)endpointContext;

    return UA_ByteString_copy(certificate, &data->localCert);
}

static const UA_ByteString*
endpointContext_getServerCertificate_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                             const void *const endpointContext) {
    if(securityPolicy == NULL || endpointContext == NULL)
        return NULL;

    const UA_SP_NONE_EndpointContextData *const data = (const UA_SP_NONE_EndpointContextData*)endpointContext;

    return &data->localCert;
}

static UA_StatusCode
endpointContext_setCertificateTrustList_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                const UA_ByteString *const trustList,
                                                void *const endpointContext) {
    if(securityPolicy == NULL || trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
endpointContext_setCertificateRevocationList_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                     const UA_ByteString *const revocationList,
                                                     void *const endpointContext) {
    if(securityPolicy == NULL || revocationList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static size_t
endpointContext_getLocalAsymSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                  const void *const endpointContext) {
    return 0;
}

static UA_StatusCode
endpointContext_compareCertificateThumbprint_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                     const void *const endpointContext,
                                                     const UA_ByteString *const certificateThumbprint) {
    if(securityPolicy == NULL || endpointContext == NULL || certificateThumbprint == NULL) {
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
    int callCounter;
} UA_SP_NONE_ChannelContextData;

static UA_StatusCode
channelContext_new_sp_none(const UA_SecurityPolicy *const securityPolicy,
                           const void *const endpointContext,
                           const UA_ByteString *const remoteCertificate,
                           void **const pp_contextData) {
    if(securityPolicy == NULL || pp_contextData == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    *pp_contextData = UA_malloc(sizeof(UA_SP_NONE_ChannelContextData));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // Initialize the channelcontext data here to sensible values
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)*pp_contextData;

    data->callCounter = 0;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode channelContext_delete_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                   void *const contextData) {
    if(securityPolicy == NULL || contextData == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    // Delete the member variables that eventually were allocated in the init method
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Call counter was %i before deletion.", data->callCounter);

    data->callCounter = 0;

    UA_free(contextData);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                const UA_ByteString *const key,
                                                void *const contextData) {
    if(securityPolicy == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalEncryptingKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                             const UA_ByteString *const key,
                                             void *const contextData) {
    if(securityPolicy == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalSigningKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                     const UA_ByteString *const iv,
                                     void *const contextData) {
    if(securityPolicy == NULL || iv == NULL) {
        fprintf(stderr, "Error while calling channelContext_setLocalIv_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                 const UA_ByteString *const key,
                                                 void *const contextData) {
    if(securityPolicy == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteEncryptingKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                              const UA_ByteString *const key,
                                              void *const contextData) {
    if(securityPolicy == NULL || key == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteSigningKey_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                      const UA_ByteString *const iv,
                                      void *const contextData) {
    if(securityPolicy == NULL || iv == NULL) {
        fprintf(stderr, "Error while calling channelContext_setRemoteIv_sp_none."
                "Null pointer passed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)contextData;

    data->callCounter++;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_compareCertificate_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                          const void *const channelContext,
                                          const UA_ByteString *const certificate) {
    if(securityPolicy == NULL || channelContext == NULL || certificate == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static size_t
channelContext_getRemoteAsymSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                  const void *const contextData) {
    return 0;
}

static size_t
channelContext_getRemoteAsymPlainTextBlockSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                       const void *const contextData) {
    return 0;
}

static size_t
channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                                                   const void *const contextData,
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
        asym_encrypt_sp_none, // .encrypt
        asym_decrypt_sp_none, // .decrypt
        asym_makeThumbprint_sp_none, // .makeThumbprint
        asym_calculatePadding_sp_none, // .calculatePadding

        0, // .minAsymmetricKeyLength
        0, // .maxAsymmetricKeyLength
        20, // .thumbprintLength

        /* Asymmetric signing module */
        {
            asym_verify_sp_none, // .verify
            asym_sign_sp_none, // .sign
            0, // .signatureSize // size_t signatureSize; in bytes
            UA_STRING_STATIC_NULL // .signatureAlgorithmUri
        }
    },

    /* Symmetric module */
    { // .symmetricModule
        sym_encrypt_sp_none, // .encrypt
        sym_decrypt_sp_none, // .decrypt
        sym_generateKey_sp_none, // .generateKey
        sym_generateNonce_sp_none, // .generateNonce 
        sym_calculatePadding_sp_none, // .calculatePadding

        /* Symmetric signing module */
        { // .signingModule
            sym_verify_sp_none, // .verify
            sym_sign_sp_none, // .sign
            0, // .signatureSize // size_t signatureSize; in bytes
            UA_STRING_STATIC_NULL // .signatureAlgorithmUri
        },

        0, // .signingKeyLength
        1, // .encryptingKeyLength
        0 // .encryptingBlockSize
    },

    { // .context
        endpointContext_new_sp_none, // .init
        endpointContext_delete_sp_none, // .deleteMembers
        endpointContext_setLocalPrivateKey_sp_none, // .setLocalPrivateKey
        endpointContext_setServerCertificate_sp_none,
        endpointContext_getServerCertificate_sp_none,
        endpointContext_setCertificateTrustList_sp_none, // .setCertificateTrustList
        endpointContext_setCertificateRevocationList_sp_none, // .setCertificateRevocationList
        endpointContext_getLocalAsymSignatureSize_sp_none, // .getLocalAsymSignatureSize
        endpointContext_compareCertificateThumbprint_sp_none
    },

    { // .channelContext
        channelContext_new_sp_none,  // .new
        channelContext_delete_sp_none, // .delete

        channelContext_setLocalSymEncryptingKey_sp_none, // .setLocalSymEncryptingKey
        channelContext_setLocalSymSigningKey_sp_none, // .setLocalSymSigningKey
        channelContext_setLocalSymIv_sp_none, // .setLocalSymIv

        channelContext_setRemoteSymEncryptingKey_sp_none, // .setRemoteSymEncryptingKey
        channelContext_setRemoteSymSigningKey_sp_none, // .setRemoteSymSigningKey
        channelContext_setRemoteSymIv_sp_none, // .setRemoteSymIv

        channelContext_compareCertificate_sp_none, // .parseRemoteCertificate

        channelContext_getRemoteAsymSignatureSize_sp_none, // .getRemoteAsymSignatureSize
        channelContext_getRemoteAsymPlainTextBlockSize_sp_none, // .getRemoteAsymPlainTextBlockSize
        channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none // .getRemoteAsymEncryptionBufferLengthOverhead
    },

    NULL // .logger
};
