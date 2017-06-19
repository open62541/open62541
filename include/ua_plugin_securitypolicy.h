/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_PLUGIN_SECURITYPOLICY_H_
#define UA_PLUGIN_SECURITYPOLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_log.h"
#include "ua_types_generated.h"

extern const UA_ByteString UA_SECURITY_POLICY_NONE_URI;

//////////////////////////
// Forward declarations //
//////////////////////////
struct UA_SecurityPolicy;
typedef struct UA_SecurityPolicy UA_SecurityPolicy;

struct UA_Channel_SecurityContext;
typedef struct UA_Channel_SecurityContext UA_Channel_SecurityContext;

struct UA_Endpoint_SecurityContext;
typedef struct UA_Endpoint_SecurityContext UA_Endpoint_SecurityContext;
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
     * \param context the context that contains the key to verify the supplied message with.
     * \param message the message to which the signature is supposed to belong.
     * \param signature the signature of the message, that should be verified.
     */
    UA_StatusCode (*const verify)(const UA_SecurityPolicy *securityPolicy,
                                  const void *context,
                                  const UA_ByteString *message,
                                  const UA_ByteString *signature);

    /**
     * Signs the given message using this policys signing
     * algorithm and the provided keys in the context.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param context the context that contains the key to sign the supplied message with.
     * \param message the message to sign.
     * \param signature an output buffer to which the signature is written. The buffer needs
     *                  to be allocated by the caller. The necessary size can be acquired with
     *                  the signatureSize attribute of this module.
     */
    UA_StatusCode (*const sign)(const UA_SecurityPolicy *securityPolicy,
                                const void *context,
                                const UA_ByteString *message,
                                UA_ByteString *signature);

        /* The signature size in bytes */
    const UA_UInt32 signatureSize;
    const UA_String signatureAlgorithmUri;
} UA_SecurityPolicySigningModule;

typedef struct
{
    /**
     * \brief Encrypt the given data in place using an asymmetric algorithm and keys.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param endpointContext the endpointContext which contains information about entropy generation.
     * \param channelContext the channelContext which contains information about the keys to encrypt data.
     * \param data the data that is encrypted. The encrypted data will overwrite the data that was supplied.
     */
    UA_StatusCode (*const encrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *endpointContext,
                                   const void *channelContext,
                                   UA_ByteString *data);
    /**
     * \brief Decrypts the given ciphertext in place using an asymmetric algorithm and key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param endpointContext the EndpointContext which contains information about the keys needed to decrypt the message.
     * \param data the data to decrypt. The decryption is done in place.
     */
    UA_StatusCode (*const decrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *endpointContext,
                                   UA_ByteString *data);
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

    /**
     * \brief Calculates the padding size for a message with the specified amount of bytes.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the channel context that is used to obtain the plaintext block size.
     *                       Has to have a remote certificate set.
     * \aram endpointContext the endpoint context that is used to get the local asym signature size.
     * \param bytesToWrite the size of the payload plus the sequence header, since both need to be encoded
     * \param paddingSize out parameter. Will contain the paddingSize byte.
     * \param extraPaddingSize out parameter. Will contain the extraPaddingSize. If no extra padding is needed, this is 0.
     * \return the total padding size consiting of high and low byte.
     */
    UA_UInt16 (*const calculatePadding)(const UA_SecurityPolicy *securityPolicy,
                                        const void *channelContext,
                                        const void *endpointContext,
                                        size_t bytesToWrite,
                                        UA_Byte *paddingSize,
                                        UA_Byte *extraPaddingSize);

    const size_t minAsymmetricKeyLength;
    const size_t maxAsymmetricKeyLength;
    const size_t thumbprintLength;

    const UA_SecurityPolicySigningModule signingModule;
} UA_SecurityPolicyAsymmetricModule;

typedef struct
{
    /**
     * \brief Encrypts the given plaintext in place using a symmetric algorithm and key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the ChannelContext to work with.
     * \param data the data to encrypt. The data will be encrypted in place.
     *             The implementation may allocate additional memory though.
     */
    UA_StatusCode (*const encrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *channelContext,
                                   UA_ByteString *data);

    /**
     * \brief Decrypts the given ciphertext using a symmetric algorithm and key.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param channelContext the ChannelContext which contains information about the keys needed to decrypt the message.
     * \param data the data to decrypt. The decryption is done in place.
     */
    UA_StatusCode (*const decrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *channelContext,
                                   UA_ByteString *data);

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
     * \param endpointContext the endpointContext that contains entropy generation data.
     * \param out pointer to a buffer to store the nonce in. Needs to be allocated by the caller.
     *            The buffer is filled with random data.
     */
    UA_StatusCode (*const generateNonce)(const UA_SecurityPolicy *securityPolicy,
                                         const void *endpointContext,
                                         UA_ByteString *out);

    /**
     * \brief Calculates the padding size for a message with the specified amount of bytes.
     *
     * \param securityPolicy the securityPolicy the function is invoked on.
     * \param bytesToWrite the size of the payload plus the sequence header, since both need to be encoded
     * \param paddingSize out parameter. Will contain the paddingSize byte.
     * \param extraPaddingSize out parameter. Will contain the extraPaddingSize. If no extra padding is needed, this is 0.
     * \return the total padding size consisting of high and low bytes.
     */
    UA_UInt16 (*const calculatePadding)(const UA_SecurityPolicy *securityPolicy,
                                        size_t bytesToWrite,
                                        UA_Byte *paddingSize,
                                        UA_Byte *extraPaddingSize);

    const UA_SecurityPolicySigningModule signingModule;

    const size_t signingKeyLength;
    const size_t encryptingKeyLength;
    const size_t encryptingBlockSize;
} UA_SecurityPolicySymmetricModule;

struct UA_Endpoint_SecurityContext {
    UA_StatusCode (*const new)(const UA_SecurityPolicy *securityPolicy,
                               const void *initData,
                               void **pp_contextData);

    UA_StatusCode (*const delete)(const UA_SecurityPolicy *securityPolicy, //TODO: remove parameter
                                  void *endpointContext);

    //TODO: Remove these functions from the public api.
    UA_StatusCode (*const setLocalPrivateKey)(const UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString *privateKey,
                                              void *endpointContext);

    UA_StatusCode (*const setServerCertificate)(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificate,
                                                void *endpointContext);

    const UA_ByteString *(*const getServerCertificate)(const UA_SecurityPolicy *securityPolicy,
                                                       const void *endpointContext);

    UA_StatusCode (*const setCertificateTrustList)(const UA_SecurityPolicy *securityPolicy,
                                                   const UA_ByteString *trustList,
                                                   void *endpointContext);

    UA_StatusCode (*const setCertificateRevocationList)(const UA_SecurityPolicy *securityPolicy,
                                                        const UA_ByteString *revocationList,
                                                        void *endpointContext);

    /**
    * \brief Gets the signature size that depends on the local private key.
    *
    * \param securityPolicy the securityPolicy the function is invoked on.
    * \param endpointContext the endpointContext that contains the certificate/key.
    * \return the size of the local signature. Returns 0 if no local certificate was set.
    */
    // TODO: move to signing module?
    size_t (*const getLocalAsymSignatureSize)(const UA_SecurityPolicy *securityPolicy, // TODO: move securityPolicy pointer to context
                                              const void *endpointContext);

    /**
     * \brief Compares the supplied certificate with the certificate in the endpoit context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param endpointContext the endpoint context data that contains the certificate to compare to.
     * \param certificateThumbprint the certificate thumbprint to compare to the one stored in the context.
     * \return if the thumbprints match UA_STATUSCODE_GOOD is returned. If they don't match
     *         or an error occured an error code is returned.
     */
    UA_StatusCode (*const compareCertificateThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                                        const void *endpointContext,
                                                        const UA_ByteString *certificateThumbprint);
};

struct UA_Channel_SecurityContext {
    /**
     * \brief This method creates a new context data object.
     *
     * The caller needs to call delete on the recieved object to free allocated memory.
     *
     * Memory is only allocated if the function succeeds so there is no need to manually free
     * the memory pointed to by *pp_contextData or to call delete in case of failure.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param remoteCertificate the remote certificate contains the remote asymmetric key.
     *                          The certificate will be verified and then stored in the context
     *                          so that its details may be accessed.
     * \param contextData the initialized contextData that is passed to functions that work
     *                    on a context.
     */
    UA_StatusCode (*const new)(const UA_SecurityPolicy *securityPolicy,
                                const void *endpointContext,
                                const UA_ByteString *remoteCertificate,
                                void **pp_contextData);

    /**
     * \brief Deletes the the security context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to delete the members of.
     */
    UA_StatusCode (*const delete)(const UA_SecurityPolicy *securityPolicy, // TODO: remove this pointer
                                  void *contextData);

    /**
     * \brief Sets the local encrypting key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the local encrypting key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymEncryptingKey)(const UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString *key,
                                                    void *contextData);

    /**
     * \brief Sets the local signing key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the local signing key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymSigningKey)(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *key,
                                                 void *contextData);

    /**
     * \brief Sets the local initialization vector in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param iv the local initialization vector to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymIv)(const UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString *iv,
                                         void *contextData);
    /**
     * \brief Sets the remote encrypting key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the remote encrypting key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setRemoteSymEncryptingKey)(const UA_SecurityPolicy *securityPolicy,
                                                     const UA_ByteString *key,
                                                     void *contextData);

    /**
     * \brief Sets the remote signing key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the remote signing key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setRemoteSymSigningKey)(const UA_SecurityPolicy *securityPolicy,
                                                  const UA_ByteString *key,
                                                  void *contextData);

    /**
     * \brief Sets the remote initialization vector in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param iv the remote initialization vector to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setRemoteSymIv)(const UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString *iv,
                                          void *contextData);

    /**
     * \brief Compares the supplied certificate with the certificate in the channel context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param channelContext the channel context data that contains the certificate to compare to.
     * \param certificate the certificate to compare to the one stored in the context.
     * \return if the certificates match UA_STATUSCODE_GOOD is returned. If they don't match
     *         or an errror occured an error code is returned.
     */
    UA_StatusCode (*const compareCertificate)(const UA_SecurityPolicy *securityPolicy,
                                              const void *channelContext,
                                              const UA_ByteString *certificate);

    /**
     * \brief Gets the signature size that depends on the remote public key.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to retrieve data from.
     * \return the size of the remote asymmetric signature. Returns 0 if no remote certificate
     *                     was set previousely.
     */
    size_t (*const getRemoteAsymSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                               const void *contextData);

    /**
     * \brief Gets the plaintext block size that depends on the remote public key.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to retrieve data from.
     * \return the size of the plain text block size when encrypting with the remote public key.
     *         Returns 0 as long as no remote certificate was set previousely.
     */
    size_t (*const getRemoteAsymPlainTextBlockSize)(const UA_SecurityPolicy *securityPolicy,
                                                    const void *contextData);

    /**
     * Gets the number of bytes that are needed by the encryption function in addition to the length of the plaintext message.
     * This is needed, since most rsa encryption methods have their own padding mechanism included. This makes the encrypted
     * message larger than the plainText, so we need to have enough room in the buffer for the overhead.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the retrieve data from.
     * \param maxEncryptionLength the maximum number of bytes that the data to encrypt can be.
     */
    size_t (*const getRemoteAsymEncryptionBufferLengthOverhead)(const UA_SecurityPolicy *securityPolicy,
                                                                const void *contextData,
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

    const UA_Endpoint_SecurityContext endpointContext;
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

#ifdef __cplusplus
}
#endif

#endif // UA_PLUGIN_SECURITYPOLICY_H_
