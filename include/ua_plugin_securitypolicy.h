/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_PLUGIN_SECURITYPOLICY_H_
#define UA_PLUGIN_SECURITYPOLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_plugin_log.h"
#include "ua_types_generated.h"

extern const UA_ByteString UA_SECURITY_POLICY_NONE_URI;

//////////////////////////
// Forward declarations //
//////////////////////////
struct UA_SecurityPolicy;
typedef struct UA_SecurityPolicy UA_SecurityPolicy;

struct UA_Channel_SecurityContext;
typedef struct UA_Channel_SecurityContext UA_Channel_SecurityContext;

struct UA_Policy_SecurityContext;
typedef struct UA_Policy_SecurityContext UA_Policy_SecurityContext;

struct UA_CertificateList;
typedef struct UA_CertificateList UA_CertificateList;
/////////////////////////////////
// End of forward declarations //
/////////////////////////////////

/**
 * This module contains the signing algorithms
 * used by a security policy.
 */
typedef struct
{
    /**
     * Verifies the signature of the message using the provided keys in the context.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channelContext that contains the key to verify the supplied message with.
     * \param message the message to which the signature is supposed to belong.
     * \param signature the signature of the message, that should be verified.
     */
    UA_StatusCode (*const verify)(const UA_SecurityPolicy *securityPolicy,
                                  const void *channelContext,
                                  const UA_ByteString *message,
                                  const UA_ByteString *signature);

    /**
     * Signs the given message using this policys signing
     * algorithm and the provided keys in the context.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channelContext that contains the key to sign the supplied message with.
     * \param message the message to sign.
     * \param signature an output buffer to which the signature is written. The buffer needs
     *                  to be allocated by the caller. The necessary size can be acquired with
     *                  the signatureSize attribute of this module.
     */
    UA_StatusCode (*const sign)(const UA_SecurityPolicy *securityPolicy,
                                const void *channelContext,
                                const UA_ByteString *message,
                                UA_ByteString *signature);

    /**
     * \brief Gets the signature size that depends on the local private key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channelContext that contains the certificate/key.
     * \return the size of the local signature. Returns 0 if no local certificate was set.
     */
    size_t (*const getLocalSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                          const void *channelContext);

    /**
     * \brief Gets the signature size that depends on the remote public key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the context to retrieve data from.
     * \return the size of the remote asymmetric signature. Returns 0 if no remote certificate
     *                     was set previousely.
     */
    size_t (*const getRemoteSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                           const void *channelContext);

        /* The signature size in bytes */
    const UA_String signatureAlgorithmUri;
} UA_SecurityPolicySigningModule;

typedef struct
{
    /**
     * \brief Encrypt the given data in place using an asymmetric algorithm and keys.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channelContext which contains information about the keys to encrypt data.
     * \param data the data that is encrypted. The encrypted data will overwrite the data that was supplied.
     */
    UA_StatusCode(*const encrypt)(const UA_SecurityPolicy *securityPolicy,
                                  const void *channelContext,
                                  UA_ByteString *data);
    /**
     * \brief Decrypts the given ciphertext in place using an asymmetric algorithm and key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channelContext which contains information about the keys needed to decrypt the message.
     * \param data the data to decrypt. The decryption is done in place.
     */
    UA_StatusCode(*const decrypt)(const UA_SecurityPolicy *securityPolicy,
                                  const void *channelContext,
                                  UA_ByteString *data);
} UA_SecurityPolicyEncryptingModule;

typedef struct
{
    /**
     * \brief Generates a thumprint for the specified certificate.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param certificate the certificate to make a thumbprint of.
     * \param thumbprint an output buffer for the resulting thumbprint. Always
                         has the length specified in the thumprintLenght in the asymmetricModule.
     */
    UA_StatusCode (*const makeThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString *certificate,
                                          UA_ByteString *thumbprint);

    const size_t minAsymmetricKeyLength;
    const size_t maxAsymmetricKeyLength;
    const size_t thumbprintLength;

    const UA_SecurityPolicyEncryptingModule encryptingModule;
    const UA_SecurityPolicySigningModule signingModule;
} UA_SecurityPolicyAsymmetricModule;

typedef struct
{
    /**
     * \brief Pseudo random function that is used to generate the symmetric keys.
     *
     * For information on what parameters this function receives in what situation,
     * refer to the OPC UA specification 1.03 Part6 Table 33
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param secret
     * \param seed
     * \param out an output to write the data to. The length defines the maximum
     *            number of output bytes that are produced.
     */
    UA_StatusCode (*const generateKey)(const UA_SecurityPolicy *securityPolicy,
                                       const UA_ByteString *secret,
                                       const UA_ByteString *seed,
                                       UA_ByteString *out);
    /**
     * \brief Random generator for generating nonces.
     * 
     * Generates a nonce.
     * 
     * \param securityPolicy the securityPolicy this function is invoked on.
     *                       Example: myPolicy->generateNonce(myPolicy, &outBuff);
     * \param policyContext the policyContext that contains entropy generation data.
     * \param out pointer to a buffer to store the nonce in. Needs to be allocated by the caller.
     *            The buffer is filled with random data.
     */
    UA_StatusCode (*const generateNonce)(const UA_SecurityPolicy *securityPolicy,
                                         const void *policyContext,
                                         UA_ByteString *out);

    const UA_SecurityPolicyEncryptingModule encryptingModule;
    const UA_SecurityPolicySigningModule signingModule;

    const size_t signingKeyLength;
    const size_t encryptingKeyLength;
    const size_t encryptingBlockSize;
} UA_SecurityPolicySymmetricModule;

/**
 * This struct contains a single certificate of a certificate list
 * and points to another element, that follows in the list.
 *
 * If this is the last element, the next pointer must be NULL.
 */
struct UA_CertificateList {
    UA_ByteString *certificate;
    UA_CertificateList *next;
};

/**
 * This struct defines initialization Data that is potentially needed by every security policy.
 * Some of these, like the revocation list for example, may be omitted (empty ByteString).
 *
 * Implementations of security policies must copy the supplied data, so that it can be freed
 * by the user application any time after the init function was called.
 */
typedef struct {
    const UA_ByteString *localPrivateKey;
    const UA_ByteString *localCertificate;
    const UA_CertificateList *certificateTrustList;
    const UA_CertificateList *certificateRevocationList;
} UA_Policy_SecurityContext_RequiredInitData;

struct UA_Policy_SecurityContext {
    /**
     * \brief Creates a new endpoint context with the supplied initialization data.
     *
     * \param securityPolicy the securityPolicy this function is invoked on.
     * \param initData the required initialization data for the endpoint context.
     * \param optInitData the optional initialization data for the endpoint context. May be NULL.
     * \param pp_contextData a pointer to a variable, where the pointer to the context will be stored.
     */
    UA_StatusCode (*const newContext)(const UA_SecurityPolicy *securityPolicy,
                                      const UA_Policy_SecurityContext_RequiredInitData *initData,
                                      const void *optInitData,
                                      void **pp_contextData);

    /**
     * \brief Deletes the supplied policy context.
     */
    UA_StatusCode (*const deleteContext)(void *policyContext);

    /**
     * \brief Gets the local certificate stored in the policyContext.
     *
     * \param policyContext the policyContext that contains the certificate.
     * \return a pointer to the certificate. It must not be modified.
     */
    const UA_ByteString *(*const getLocalCertificate)(const void *policyContext);

    /**
     * \brief Compares the supplied certificate with the certificate in the endpoit context.
     *
     * \param policyContext the policyContext data that contains the certificate to compare to.
     * \param certificateThumbprint the certificate thumbprint to compare to the one stored in the context.
     * \return if the thumbprints match UA_STATUSCODE_GOOD is returned. If they don't match
     *         or an error occured an error code is returned.
     */
    UA_StatusCode (*const compareCertificateThumbprint)(const void *policyContext,
                                                        const UA_ByteString *certificateThumbprint);
};

struct UA_Channel_SecurityContext {
    /**
     * \brief This method creates a new context data object.
     *
     * The caller needs to call delete on the recieved object to free allocated memory.
     *
     * Memory is only allocated if the function succeeds so there is no need to manually free
     * the memory pointed to by *pp_channelContext or to call delete in case of failure.
     *
     * \param policyContext the policy context of the endpoint that is connected to. It will be
     *                      stored in the channelContext for further access by the policy.
     * \param remoteCertificate the remote certificate contains the remote asymmetric key.
     *                          The certificate will be verified and then stored in the context
     *                          so that its details may be accessed.
     * \param pp_channelContext the initialized channelContext that is passed to functions that work
     *                    on a context.
     */
    UA_StatusCode (*const newContext)(const void *policyContext,
                                      const UA_ByteString *remoteCertificate,
                                      void **pp_channelContext);

    /**
     * \brief Deletes the the security context.
     *
     * \param channelContext the context to delete the members of.
     */
    UA_StatusCode (*const deleteContext)(void *channelContext);

    /**
     * \brief Sets the local encrypting key in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param key the local encrypting key to store in the context.
     */
    UA_StatusCode (*const setLocalSymEncryptingKey)(void *channelContext,
                                                    const UA_ByteString *key);

    /**
     * \brief Sets the local signing key in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param key the local signing key to store in the context.
     */
    UA_StatusCode (*const setLocalSymSigningKey)(void *channelContext,
                                                 const UA_ByteString *key);

    /**
     * \brief Sets the local initialization vector in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param iv the local initialization vector to store in the context.
     */
    UA_StatusCode (*const setLocalSymIv)(void *channelContext,
                                         const UA_ByteString *iv);
    /**
     * \brief Sets the remote encrypting key in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param key the remote encrypting key to store in the context.
     */
    UA_StatusCode (*const setRemoteSymEncryptingKey)(void *channelContext,
                                                     const UA_ByteString *key);

    /**
     * \brief Sets the remote signing key in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param key the remote signing key to store in the context.
     */
    UA_StatusCode (*const setRemoteSymSigningKey)(void *channelContext,
                                                  const UA_ByteString *key);

    /**
     * \brief Sets the remote initialization vector in the supplied context.
     *
     * \param channelContext the context to work on.
     * \param iv the remote initialization vector to store in the context.
     */
    UA_StatusCode (*const setRemoteSymIv)(void *channelContext,
                                          const UA_ByteString *iv);

    /**
     * \brief Compares the supplied certificate with the certificate in the channel context.
     *
     * \param channelContext the channel context data that contains the certificate to compare to.
     * \param certificate the certificate to compare to the one stored in the context.
     * \return if the certificates match UA_STATUSCODE_GOOD is returned. If they don't match
     *         or an errror occured an error code is returned.
     */
    UA_StatusCode (*const compareCertificate)(const void *channelContext,
                                              const UA_ByteString *certificate);

    /**
     * \brief Gets the plaintext block size that depends on the remote public key.
     *
     * \param channelContext the context to retrieve data from.
     * \return the size of the plain text block size when encrypting with the remote public key.
     *         Returns 0 as long as no remote certificate was set previousely.
     */
    size_t (*const getRemoteAsymPlainTextBlockSize)(const void *channelContext);

    /**
     * Gets the number of bytes that are needed by the encryption function in addition to the length of the plaintext message.
     * This is needed, since most rsa encryption methods have their own padding mechanism included. This makes the encrypted
     * message larger than the plainText, so we need to have enough room in the buffer for the overhead.
     *
     * \param channelContext the retrieve data from.
     * \param maxEncryptionLength the maximum number of bytes that the data to encrypt can be.
     */
    size_t (*const getRemoteAsymEncryptionBufferLengthOverhead)(const void *channelContext,
                                                                size_t maxEncryptionLength);
};


/**
 * \brief This struct describes a security policy. All functions and modules need to be implemented.
 */
struct UA_SecurityPolicy {
    /* The policy uri that identifies the implemented algorithms */
    const UA_ByteString policyUri;

    const UA_SecurityPolicyAsymmetricModule asymmetricModule;
    const UA_SecurityPolicySymmetricModule symmetricModule;

    const UA_Policy_SecurityContext policyContext;
    const UA_Channel_SecurityContext channelContext;

    UA_Logger logger;
};

/**
 * Holds an endpoint description and the corresponding security policy
 * Also holds the context for the endpoint.
 */
typedef struct {
    UA_SecurityPolicy *securityPolicy;
    void *securityContext;
    UA_EndpointDescription endpointDescription;
} UA_Endpoint;

typedef struct {
    size_t count;
    UA_Endpoint *endpoints;
} UA_Endpoints;

/**
 * \brief Creates a new certificate list with one element.
 *
 * \param certificate The first certificate to be stored in the list. (will be copied)
 * \return returns a pointer to the first list element.
 */
UA_CertificateList *UA_CertificateList_new(const UA_ByteString *certificate);

/**
 * \brief Prepends a certificate to the list.
 *
 * \param list a pointer to the first list element pointer. Will be modified, since we are prepending.
 * \param certificate the certificate to store in the list. (will be copied)
 */
UA_StatusCode UA_CertificateList_prepend(UA_CertificateList **list, const UA_ByteString *certificate);

/**
 * \brief Deletes the supplied certificate list. This frees all elements and the contained data.
 *
 * \param list the list to delete
 */
void UA_CertificateList_delete(UA_CertificateList *list);

#ifdef __cplusplus
}
#endif

#endif // UA_PLUGIN_SECURITYPOLICY_H_
