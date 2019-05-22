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

#include <open62541/plugin/log.h>
#include <open62541/plugin/pki.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

extern UA_EXPORT const UA_ByteString UA_SECURITY_POLICY_NONE_URI;

struct UA_SecurityPolicy;
typedef struct UA_SecurityPolicy UA_SecurityPolicy;

/**
 * SecurityPolicy Interface Definition
 * ----------------------------------- */

typedef struct {
    UA_String uri;

    /* Verifies the signature of the message using the provided keys in the context.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the channelContext that contains the key to verify
     *                       the supplied message with.
     * @param message the message to which the signature is supposed to belong.
     * @param signature the signature of the message, that should be verified. */
    UA_StatusCode (*verify)(const UA_SecurityPolicy *securityPolicy,
                            void *channelContext, const UA_ByteString *message,
                            const UA_ByteString *signature) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Signs the given message using this policys signing algorithm and the
     * provided keys in the context.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the channelContext that contains the key to sign
     *                       the supplied message with.
     * @param message the message to sign.
     * @param signature an output buffer to which the signature is written. The
     *                  buffer needs to be allocated by the caller. The
     *                  necessary size can be acquired with the signatureSize
     *                  attribute of this module. */
    UA_StatusCode (*sign)(const UA_SecurityPolicy *securityPolicy,
                          void *channelContext, const UA_ByteString *message,
                          UA_ByteString *signature) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Gets the signature size that depends on the local (private) key.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the channelContext that contains the
     *                       certificate/key.
     * @return the size of the local signature. Returns 0 if no local
     *         certificate was set. */
    size_t (*getLocalSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                    const void *channelContext);

    /* Gets the signature size that depends on the remote (public) key.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the size of the remote signature. Returns 0 if no
     *         remote certificate was set previousely. */
    size_t (*getRemoteSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                     const void *channelContext);

    /* Gets the local signing key length.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the length of the signing key in bytes. Returns 0 if no length can be found.
     */
    size_t (*getLocalKeyLength)(const UA_SecurityPolicy *securityPolicy,
                                const void *channelContext);

    /* Gets the local signing key length.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the length of the signing key in bytes. Returns 0 if no length can be found.
     */
    size_t (*getRemoteKeyLength)(const UA_SecurityPolicy *securityPolicy,
                                 const void *channelContext);
} UA_SecurityPolicySignatureAlgorithm;

typedef struct {
    UA_String uri;

    /* Encrypt the given data in place using an asymmetric algorithm and keys.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the channelContext which contains information about
     *                       the keys to encrypt data.
     * @param data the data that is encrypted. The encrypted data will overwrite
     *             the data that was supplied. */
    UA_StatusCode (*encrypt)(const UA_SecurityPolicy *securityPolicy,
                             void *channelContext,
                             UA_ByteString *data) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Decrypts the given ciphertext in place using an asymmetric algorithm and
     * key.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the channelContext which contains information about
     *                       the keys needed to decrypt the message.
     * @param data the data to decrypt. The decryption is done in place. */
    UA_StatusCode (*decrypt)(const UA_SecurityPolicy *securityPolicy,
                             void *channelContext,
                             UA_ByteString *data) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Returns the length of the key used locally to encrypt messages in bits
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the length of the local key. Returns 0 if no
     *         key length is known. */
    size_t (*getLocalKeyLength)(const UA_SecurityPolicy *securityPolicy,
                                const void *channelContext);

    /* Returns the length of the key used remotely to encrypt messages in bits
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the length of the remote key. Returns 0 if no
     *         key length is known. */
    size_t (*getRemoteKeyLength)(const UA_SecurityPolicy *securityPolicy,
                                 const void *channelContext);

    /* Returns the size of encrypted blocks used by the local encryption algorithm.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the size of encrypted blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getLocalBlockSize)(const UA_SecurityPolicy *securityPolicy,
                                const void *channelContext);

    /* Returns the size of encrypted blocks used by the remote encryption algorithm.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the size of encrypted blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getRemoteBlockSize)(const UA_SecurityPolicy *securityPolicy,
                                 const void *channelContext);

    /* Returns the size of plaintext blocks used by the local encryption algorithm.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the size of plaintext blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getLocalPlainTextBlockSize)(const UA_SecurityPolicy *securityPolicy,
                                         const void *channelContext);

    /* Returns the size of plaintext blocks used by the remote encryption algorithm.
     *
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param channelContext the context to retrieve data from.
     * @return the size of plaintext blocks in bytes. Returns 0 if no key length is known.
     */
    size_t (*getRemotePlainTextBlockSize)(const UA_SecurityPolicy *securityPolicy,
                                          const void *channelContext);
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
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param certificate the certificate to make a thumbprint of.
     * @param thumbprint an output buffer for the resulting thumbprint. Always
     *                   has the length specified in the thumbprintLength in the
     *                   asymmetricModule. */
    UA_StatusCode (*makeCertificateThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString *certificate,
                                               UA_ByteString *thumbprint)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Compares the supplied certificate with the certificate in the endpoit context.
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
     * @param securityPolicy the securityPolicy the function is invoked on.
     * @param secret
     * @param seed
     * @param out an output to write the data to. The length defines the maximum
     *            number of output bytes that are produced. */
    UA_StatusCode (*generateKey)(const UA_SecurityPolicy *securityPolicy,
                                 const UA_ByteString *secret,
                                 const UA_ByteString *seed, UA_ByteString *out)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

    /* Random generator for generating nonces.
     *
     * @param securityPolicy the securityPolicy this function is invoked on.
     *                       Example: myPolicy->generateNonce(myPolicy,
     *                       &outBuff);
     * @param out pointer to a buffer to store the nonce in. Needs to be
     *            allocated by the caller. The buffer is filled with random
     *            data. */
    UA_StatusCode (*generateNonce)(const UA_SecurityPolicy *securityPolicy,
                                   UA_ByteString *out)
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
    UA_ByteString policyUri;

    /* The local certificate is specific for each SecurityPolicy since it
     * depends on the used key length. */
    UA_ByteString localCertificate;

    /* Function pointers grouped into modules */
    UA_SecurityPolicyAsymmetricModule asymmetricModule;
    UA_SecurityPolicySymmetricModule symmetricModule;
    UA_SecurityPolicySignatureAlgorithm certificateSigningAlgorithm;
    UA_SecurityPolicyChannelModule channelModule;
    UA_CertificateVerification *certificateVerification;

    const UA_Logger *logger;

    /* Updates the ApplicationInstanceCertificate and the corresponding private
     * key at runtime. */
    UA_StatusCode (*updateCertificateAndPrivateKey)(UA_SecurityPolicy *policy,
                                                    const UA_ByteString newCertificate,
                                                    const UA_ByteString newPrivateKey);

    /* Deletes the dynamic content of the policy */
    void (*deleteMembers)(UA_SecurityPolicy *policy);
};

/* Gets the number of bytes that are needed by the encryption function in
 * addition to the length of the plaintext message. This is needed, since
 * most RSA encryption methods have their own padding mechanism included.
 * This makes the encrypted message larger than the plainText, so we need to
 * have enough room in the buffer for the overhead.
 *
 * @param securityPolicy the algorithms to use.
 * @param channelContext the retrieve data from.
 * @param maxEncryptionLength the maximum number of bytes that the data to
 *                            encrypt can be. */
size_t
UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(const UA_SecurityPolicy *securityPolicy,
                                                              const void *channelContext,
                                                              size_t maxEncryptionLength);

/* Gets the a pointer to the context of a security policy supported by the
 * server matched by the security policy uri.
 *
 * @param server the server context.
 * @param securityPolicyUri the security policy to get the context of. */
UA_SecurityPolicy *
UA_SecurityPolicy_getSecurityPolicyByUri(const UA_Server *server,
                                         UA_ByteString *securityPolicyUri);

_UA_END_DECLS

#endif /* UA_PLUGIN_SECURITYPOLICY_H_ */
