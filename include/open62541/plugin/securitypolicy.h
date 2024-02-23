/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_SECURITYPOLICY_H_
#define UA_PLUGIN_SECURITYPOLICY_H_

#include <open62541/util.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/certificategroup.h>

_UA_BEGIN_DECLS

extern UA_EXPORT const UA_String UA_SECURITY_POLICY_NONE_URI;

struct UA_SecurityPolicy;
typedef struct UA_SecurityPolicy UA_SecurityPolicy;

/**
 * SecurityPolicy 
 * -------------- */

typedef struct {
    UA_String uri;

    /* Verifies the signature of the message using the provided keys in the context.
     *
     * @param channelContext the channelContext that contains the key to verify
     *                       the supplied message with.
     * @param message the message to which the signature is supposed to belong.
     * @param signature the signature of the message, that should be verified. */
    UA_StatusCode (*verify)(void *channelContext, const UA_ByteString *message,
                            const UA_ByteString *signature) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Signs the given message using this policys signing algorithm and the
     * provided keys in the context.
     *
     * @param channelContext the channelContext that contains the key to sign
     *                       the supplied message with.
     * @param message the message to sign.
     * @param signature an output buffer to which the signature is written. The
     *                  buffer needs to be allocated by the caller. The
     *                  necessary size can be acquired with the signatureSize
     *                  attribute of this module. */
    UA_StatusCode (*sign)(void *channelContext, const UA_ByteString *message,
                          UA_ByteString *signature) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Gets the signature size that depends on the local (private) key.
     *
     * @param channelContext the channelContext that contains the
     *                       certificate/key.
     * @return the size of the local signature. Returns 0 if no local
     *         certificate was set. */
    size_t (*getLocalSignatureSize)(const void *channelContext);

    /* Gets the signature size that depends on the remote (public) key.
     *
     * @param channelContext the context to retrieve data from.
     * @return the size of the remote signature. Returns 0 if no
     *         remote certificate was set previousely. */
    size_t (*getRemoteSignatureSize)(const void *channelContext);

    /* Gets the local signing key length.
     *
     * @param channelContext the context to retrieve data from.
     * @return the length of the signing key in bytes. Returns 0 if no length can be found.
     */
    size_t (*getLocalKeyLength)(const void *channelContext);

    /* Gets the local signing key length.
     *
     * @param channelContext the context to retrieve data from.
     * @return the length of the signing key in bytes. Returns 0 if no length can be found.
     */
    size_t (*getRemoteKeyLength)(const void *channelContext);
} UA_SecurityPolicySignatureAlgorithm;

typedef struct {
    UA_String uri;

    /* Encrypt the given data in place. For asymmetric encryption, the block
     * size for plaintext and cypher depend on the remote key (certificate).
     *
     * @param channelContext the channelContext which contains information about
     *                       the keys to encrypt data.
     * @param data the data that is encrypted. The encrypted data will overwrite
     *             the data that was supplied. */
    UA_StatusCode (*encrypt)(void *channelContext,
                             UA_ByteString *data) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Decrypts the given ciphertext in place. For asymmetric encryption, the
     * block size for plaintext and cypher depend on the local private key.
     *
     * @param channelContext the channelContext which contains information about
     *                       the keys needed to decrypt the message.
     * @param data the data to decrypt. The decryption is done in place. */
    UA_StatusCode (*decrypt)(void *channelContext,
                             UA_ByteString *data) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Returns the length of the key used to encrypt messages in bits. For
     * asymmetric encryption the key length is for the local private key.
     *
     * @param channelContext the context to retrieve data from.
     * @return the length of the local key. Returns 0 if no
     *         key length is known. */
    size_t (*getLocalKeyLength)(const void *channelContext);

    /* Returns the length of the key to encrypt messages in bits. Depends on the
     * key (certificate) from the remote side.
     *
     * @param channelContext the context to retrieve data from.
     * @return the length of the remote key. Returns 0 if no
     *         key length is known. */
    size_t (*getRemoteKeyLength)(const void *channelContext);

    /* Returns the size of encrypted blocks for sending. For asymmetric
     * encryption this depends on the remote key (certificate). For symmetric
     * encryption the local and remote encrypted block size are identical.
     *
     * @param channelContext the context to retrieve data from.
     * @return the size of encrypted blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getRemoteBlockSize)(const void *channelContext);

    /* Returns the size of plaintext blocks for sending. For asymmetric
     * encryption this depends on the remote key (certificate). For symmetric
     * encryption the local and remote plaintext block size are identical.
     *
     * @param channelContext the context to retrieve data from.
     * @return the size of plaintext blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getRemotePlainTextBlockSize)(const void *channelContext);
} UA_SecurityPolicyEncryptionAlgorithm;

typedef struct {
    /* The algorithm used to sign and verify certificates. */
    UA_SecurityPolicySignatureAlgorithm signatureAlgorithm;

    /* The algorithm used to encrypt and decrypt messages. */
    UA_SecurityPolicyEncryptionAlgorithm encryptionAlgorithm;

} UA_SecurityPolicyCryptoModule;

typedef struct {
    /* Generates a thumbprint for the specified certificate.
     *
     * @param certificate the certificate to make a thumbprint of.
     * @param thumbprint an output buffer for the resulting thumbprint. Always
     *                   has the length specified in the thumbprintLength in the
     *                   asymmetricModule. */
    UA_StatusCode (*makeCertificateThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString *certificate,
                                               UA_ByteString *thumbprint)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Compares the supplied certificate with the certificate in the endpoint context.
     *
     * @param securityPolicy the policy data that contains the certificate
     *                       to compare to.
     * @param certificateThumbprint the certificate thumbprint to compare to the
     *                              one stored in the context.
     * @return if the thumbprints match UA_STATUSCODE_GOOD is returned. If they
     *         don't match or an error occurred an error code is returned. */
    UA_StatusCode (*compareCertificateThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                                  const UA_ByteString *certificateThumbprint)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    UA_SecurityPolicyCryptoModule cryptoModule;
} UA_SecurityPolicyAsymmetricModule;

typedef struct {
    /* Pseudo random function that is used to generate the symmetric keys.
     *
     * For information on what parameters this function receives in what situation,
     * refer to the OPC UA specification 1.03 Part6 Table 33
     *
     * @param policyContext The context of the policy instance
     * @param secret
     * @param seed
     * @param out an output to write the data to. The length defines the maximum
     *            number of output bytes that are produced. */
    UA_StatusCode (*generateKey)(void *policyContext, const UA_ByteString *secret,
                                 const UA_ByteString *seed, UA_ByteString *out)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Random generator for generating nonces.
     *
     * @param policyContext The context of the policy instance
     * @param out pointer to a buffer to store the nonce in. Needs to be
     *            allocated by the caller. The buffer is filled with random
     *            data. */
    UA_StatusCode (*generateNonce)(void *policyContext, UA_ByteString *out)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /*
     * The length of the nonce used in the SecureChannel as specified in the standard.
     */
    size_t secureChannelNonceLength;

    UA_SecurityPolicyCryptoModule cryptoModule;
} UA_SecurityPolicySymmetricModule;

typedef struct {
    /* This method creates a new context data object.
     *
     * The caller needs to call delete on the received object to free allocated
     * memory. Memory is only allocated if the function succeeds so there is no
     * need to manually free the memory pointed to by *channelContext or to
     * call delete in case of failure.
     *
     * @param securityPolicy the policy context of the endpoint that is connected
     *                       to. It will be stored in the channelContext for
     *                       further access by the policy.
     * @param remoteCertificate the remote certificate contains the remote
     *                          asymmetric key. The certificate will be verified
     *                          and then stored in the context so that its
     *                          details may be accessed.
     * @param channelContext the initialized channelContext that is passed to
     *                       functions that work on a context. */
    UA_StatusCode (*newContext)(const UA_SecurityPolicy *securityPolicy,
                                const UA_ByteString *remoteCertificate,
                                void **channelContext)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Deletes the the security context. */
    void (*deleteContext)(void *channelContext);

    /* Sets the local encrypting key in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param key the local encrypting key to store in the context. */
    UA_StatusCode (*setLocalSymEncryptingKey)(void *channelContext,
                                              const UA_ByteString *key)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Sets the local signing key in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param key the local signing key to store in the context. */
    UA_StatusCode (*setLocalSymSigningKey)(void *channelContext,
                                           const UA_ByteString *key)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Sets the local initialization vector in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param iv the local initialization vector to store in the context. */
    UA_StatusCode (*setLocalSymIv)(void *channelContext,
                                   const UA_ByteString *iv)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Sets the remote encrypting key in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param key the remote encrypting key to store in the context. */
    UA_StatusCode (*setRemoteSymEncryptingKey)(void *channelContext,
                                               const UA_ByteString *key)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Sets the remote signing key in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param key the remote signing key to store in the context. */
    UA_StatusCode (*setRemoteSymSigningKey)(void *channelContext,
                                            const UA_ByteString *key)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Sets the remote initialization vector in the supplied context.
     *
     * @param channelContext the context to work on.
     * @param iv the remote initialization vector to store in the context. */
    UA_StatusCode (*setRemoteSymIv)(void *channelContext,
                                    const UA_ByteString *iv)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Compares the supplied certificate with the certificate in the channel
     * context.
     *
     * @param channelContext the channel context data that contains the
     *                       certificate to compare to.
     * @param certificate the certificate to compare to the one stored in the context.
     * @return if the certificates match UA_STATUSCODE_GOOD is returned. If they
     *         don't match or an errror occurred an error code is returned. */
    UA_StatusCode (*compareCertificate)(const void *channelContext,
                                        const UA_ByteString *certificate)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;
} UA_SecurityPolicyChannelModule;

struct UA_SecurityPolicy {
    /* Additional data */
    void *policyContext;

    /* The policy uri that identifies the implemented algorithms */
    UA_String policyUri;

    /* Value indicating the crypto strength of the policy, with zero for deprecated or none */
    UA_Byte securityLevel;

    /* The local certificate is specific for each SecurityPolicy since it
     * depends on the used key length. */
    UA_ByteString localCertificate;

    /* Function pointers grouped into modules */
    UA_SecurityPolicyAsymmetricModule asymmetricModule;
    UA_SecurityPolicySymmetricModule symmetricModule;
    UA_SecurityPolicySignatureAlgorithm certificateSigningAlgorithm;
    UA_SecurityPolicyChannelModule channelModule;

    const UA_Logger *logger;

    /* Updates the ApplicationInstanceCertificate and the corresponding private
     * key at runtime. */
    UA_StatusCode (*updateCertificateAndPrivateKey)(UA_SecurityPolicy *policy,
                                                    const UA_ByteString newCertificate,
                                                    const UA_ByteString newPrivateKey);

    /* Deletes the dynamic content of the policy */
    void (*clear)(UA_SecurityPolicy *policy);
};

/**
 * PubSub SecurityPolicy
 * ---------------------
 *
 * For PubSub encryption, the message nonce is part of the (unencrypted)
 * SecurityHeader. The nonce is required for the de- and encryption and has to
 * be set in the channel context before de/encrypting. */

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
struct UA_PubSubSecurityPolicy;
typedef struct UA_PubSubSecurityPolicy UA_PubSubSecurityPolicy;

struct UA_PubSubSecurityPolicy {
    UA_String policyUri; /* The policy uri that identifies the implemented
                          * algorithms */
    UA_SecurityPolicySymmetricModule symmetricModule;

    /* Create the context for the WriterGroup. The keys and nonce can be NULL
     * here. Then they have to be set before the first encryption or signing
     * operation. */
    UA_StatusCode
    (*newContext)(void *policyContext,
                  const UA_ByteString *signingKey,
                  const UA_ByteString *encryptingKey,
                  const UA_ByteString *keyNonce,
                  void **wgContext);

    /* Delete the WriterGroup SecurityPolicy context */
    void (*deleteContext)(void *wgContext);

    /* Set the keys and nonce for the WriterGroup. This is returned from the
     * GetSecurityKeys method of a Security Key Service (SKS). Otherwise, set
     * manually via out-of-band transmission of the keys. */
    UA_StatusCode
    (*setSecurityKeys)(void *wgContext,
                       const UA_ByteString *signingKey,
                       const UA_ByteString *encryptingKey,
                       const UA_ByteString *keyNonce)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* The nonce is contained in the NetworkMessage SecurityHeader. Set before
     * each en-/decryption step. */
    UA_StatusCode
    (*setMessageNonce)(void *wgContext,
                       const UA_ByteString *nonce)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    const UA_Logger *logger;

    /* Deletes the dynamic content of the policy */
    void (*clear)(UA_PubSubSecurityPolicy *policy);
    void *policyContext;
};

#endif

_UA_END_DECLS

#endif /* UA_PLUGIN_SECURITYPOLICY_H_ */
