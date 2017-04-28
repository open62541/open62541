/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SECURITYPOLICY_H_
#define UA_SECURITYPOLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_securitycontext.h"
#include "ua_securitypolicy_fwd.h"

extern const UA_ByteString UA_SECURITY_POLICY_NONE_URI;

typedef struct
{
    /**
     * Verifies the signature of the message using the provided certificate.
     *
     * \param message the message to verify.
     * \param context the context that contains the key to verify the supplied message with.
     */
    UA_StatusCode(*const verify)(const UA_SecurityPolicy *securityPolicy,
                                 const void *context,
                                 const UA_ByteString *message,
                                 const UA_ByteString *signature);

    /**
     * Signs the given message using this policys signing algorithm and the provided certificate.
     *
     * \param message the message to sign.
     * \param context the context that contains the key to sign the supplied message with.
     * \param signature an output buffer to which the signed message is written.
     */
    UA_StatusCode(*const sign)(const UA_SecurityPolicy *securityPolicy,
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
     * \param policyContext the policyContext which contains information about entropy generation.
     * \param channelContext the channelContext which contains information about the keys to encrypt data.
     * \param data the data that is encrypted. The encrypted data will overwrite the data that was supplied.
     */
    UA_StatusCode (*const encrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *endpointContext,
                                   const void *channelContext,
                                   const UA_ByteString *data);
    /**
     * \brief Decrypts the given cyphertext using an asymmetric algorithm and key.
     *
     * \param securityContext the SecurityContext which contains information about the keys needed to decrypt the message.
     * \param data the data to decrypt. The decryption is done in place.
     */
    UA_StatusCode (*const decrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *endpointContext,
                                   UA_ByteString *data);
    /**
     * Generates a thumprint for the specified certificate using a SHA1 digest
     *
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
     * \brief Encrypts the given plaintext using a symmetric algorithm and key.
     *
     * \param securityContext the SecurityContext to work on.
     * \param data the data to encrypt. The data will be encrypted in place.
     *             The implementation may allocate additional memory though.
     */
    UA_StatusCode (*const encrypt)(const UA_SecurityPolicy *securityPolicy,
                                   const void *channelContext,
                                   UA_ByteString *data);

    /**
     * \brief Decrypts the given ciphertext using a symmetric algorithm and key.
     *
     * \param securityContext the SecurityContext which contains information about the keys needed to decrypt the message.
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
     * \param securityPolicy the securityPolicy this function is invoked on. Example: myPolicy->generateNonce(myPolicy, &outBuff);
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

/**
 * \brief This struct describes a security policy. All functions and modules need to be implemented.
 */
struct UA_SecurityPolicy {
    /* The policy uri that identifies the implemented algorithms */
    const UA_ByteString policyUri;

    /**
     * \brief Verifies the certificate using the trust list and revocation list in the policy context
     *
     * \param policyContext the policy context that contains the revocation and trust lists.
     * \param channelContext the channel context that contains the already parsed certificate.
     */
    UA_StatusCode (*const verifyCertificate)(const UA_SecurityPolicy *securityPolicy,
                                             const void *endpointContext,
                                             const void *channelContext);

    const UA_SecurityPolicyAsymmetricModule asymmetricModule;
    const UA_SecurityPolicySymmetricModule symmetricModule;

    const UA_Endpoint_SecurityContext endpointContext;
    const UA_Channel_SecurityContext channelContext;

    /**
     * Deletes the members (namely the context) of the security policy.
     * This method is only safe when passing the security policy this method is invoked on to itself.
     * The implementer of this method should somehow assert this.
     *
     * \param securityPolicy the security policy to delete the members of. Should only be the security policy the method is invoked on.
     *                       example: mySecurityPolicy.deleteMembers(&mySecurityPolicy);
     */
    UA_StatusCode (*const deleteMembers)(UA_SecurityPolicy *securityPolicy);

    /**
     * Initializes the security policy.
     *
     * \param securityPolicy
     * \param logger
     * \param initData pointer to memory area where data required for initialization is stored.
     *                 Usually the policy will supply a struct, which can be filled and passed.
     *                 The securityPolicy may acceppt NULL and use default. This depends on implementation.
     */
    UA_StatusCode (*const init)(UA_SecurityPolicy *securityPolicy,
                                UA_Logger logger,
                                void *initData);

    UA_Logger logger;

    void *endpointContextData; // TODO: move this to endpoints!
};

/**
 * \brief A helper struct that makes it easier passing around the available policies.
 */
typedef struct {
    /** The number of policies */
    size_t count;
    /** The policy array */
    UA_SecurityPolicy *policies;
} UA_SecurityPolicies;

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYPOLICY_H_
