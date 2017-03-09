/*
* Copyright (C) 2014 the contributors as stated in the AUTHORS file
*
* This file is part of open62541. open62541 is free software: you can
* redistribute it and/or modify it under the terms of the GNU Lesser General
* Public License, version 3 (as published by the Free Software Foundation) with
* a static linking exception as stated in the LICENSE file provided with
* open62541.
*
* open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*/

#ifndef UA_SECURITYPOLICY_H_
#define UA_SECURITYPOLICY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_securitycontext.h"

struct _UA_SecurityPolicy;
typedef struct _UA_SecurityPolicy UA_SecurityPolicy;

typedef struct
{
    /**
     * Verifies the signature of the message using the provided certificate.
     *
     * \param message the message to verify.
     * \param context the context that contains the key to verify the supplied message with.
     */
    UA_StatusCode(*const verify)(const UA_ByteString* const message,
                                 const UA_ByteString* const signature,
                                 const void* const context);

    /**
     * Signs the given message using this policys signing algorithm and the provided certificate.
     *
     * \param message the message to sign.
     * \param context the context that contains the key to sign the supplied message with.
     * \param signature an output buffer to which the signed message is written.
     */
    UA_StatusCode(*const sign)(const UA_ByteString* const message,
                               const void* const context,
                               UA_ByteString* const signature);

        /* The signature size in bytes */
    size_t signatureSize;
} UA_SecurityPolicySigningModule;

typedef struct
{
    /**
     * Encrypt the given plaintext using an asymmetric algorithm and keys.
     *
     * \param plainText the text to encrypt.
     * \param securityContext the SecurityContext which contains information about the keys needed to decrypt the message.
     * \param cipher an output buffer to which the encrypted message is written.
     */
    UA_StatusCode (*const encrypt)(const UA_ByteString* const plainText,
                                   const UA_Policy_SecurityContext* const securityContext,
                                   UA_ByteString* const cipher);
    /**
     * Decrypts the given cyphertext using an asymmetric algorithm and key.
     *
     * \param cipher the ciphertext to decrypt.
     * \param securityContext the SecurityContext which contains information about the keys needed to decrypt the message.
     * \param decrypted an output buffer to which the decrypted message is written.
     */
    UA_StatusCode (*const decrypt)(const UA_ByteString* const cipher,
                                   const UA_Policy_SecurityContext* const securityContext,
                                   UA_ByteString* const decrypted);
    /**
     * Generates a thumprint for the specified certificate using a SHA1 digest
     *
     * \param certificate the certificate to make a thumbprint of.
     * \param thumbprint an output buffer for the resulting thumbprint. Always
                         has the length specified in the thumprintLenght in the asymmetricModule.
     */
    UA_StatusCode (*const makeThumbprint)(const UA_ByteString* const certificate,
                                          UA_ByteString* const thumbprint);

    /**
     * Calculates the padding size for a message with the specified amount of bytes.
     *
     * \param bytesToWrite
     */
    UA_StatusCode (*const calculatePadding)(const size_t bytesToWrite,
                                            uint16_t* const paddingSize,
                                            UA_Boolean* const extraPadding);

    const size_t thumbprintLength;
    const UA_SecurityPolicySigningModule signingModule;
} UA_SecurityPolicyAsymmetricModule;

typedef struct
{
    /**
     * Encrypts the given plaintext using a symmetric algorithm and key.
     *
     * \param plainText the text to encrypt.
     * \param cipher an output buffer to which the encrypted message is written.
     */
    UA_StatusCode (*const encrypt)(const UA_ByteString* const plainText,
                                   const UA_Channel_SecurityContext* const securityContext,
                                   UA_ByteString* const cipher);

    /**
     * Decrypts the given ciphertext using a symmetric algorithm and key.
     *
     * \param cipher the ciphertext to decrypt.
     * \param decrypted an output buffer to which the decrypted message is written.
     */
    UA_StatusCode (*const decrypt)(const UA_ByteString* const cipher,
                                   const UA_Channel_SecurityContext* const securityContext,
                                   UA_ByteString* const decrypted);

    /**
     * Pseudo random function that is used to generate the symmetric keys.
     *
     * For information on what parameters this function receives in what situation,
     * refer to the OPC UA specification 1.03 Part6 Table 33
     *
     * \param secret
     * \param seed
     * \param length the number of bytes to return
     * \param out an output to write the data to. The length needs to be equal to the parameter length.
     */
    UA_StatusCode (*const generateKey)(const UA_ByteString* const secret,
                                       const UA_ByteString* const seed,
                                       const size_t length,
                                       UA_ByteString* const out);
    /**
     * Random generator for generating nonces.
     * 
     * Generates a nonce. The length will be the same as the symmetric encryptingKeyLength
     * as specified in the OPC UA Specification Part 4 - Services 1.03 on page 20
     * 
     * \param securityPolicy the securityPolicy this function is invoked on. Example: myPolicy->generateNonce(myPolicy, &outBuff);
     * \param out pointer to a buffer to store the nonce in. Needs to be allocated by the caller.
     *            The size must be equal to the symmetric encrypting key length.
     */
    UA_StatusCode (*const generateNonce)(const UA_SecurityPolicy* const securityPolicy,
                                         UA_ByteString* const out);

    const UA_SecurityPolicySigningModule signingModule;

    const size_t signingKeyLength;
    const size_t encryptingKeyLength;
    const size_t encryptingBlockSize;
} UA_SecurityPolicySymmetricModule;

/**
 * \brief This struct describes a security policy. All functions and modules need to be implemented.
 */
struct _UA_SecurityPolicy
{
    /* The policy uri that identifies the implemented algorithms */
    const UA_ByteString policyUri;

    /**
     * Verifies the certificate using the trust list and revocation list in the security configuration
     *
     * \param certificate the certificate to verify.
     * \param securityConfig the security configuration which contains the trust list and the revocation list.
     */
    UA_StatusCode (*const verifyCertificate)(const UA_ByteString* const certificate,
                                             const UA_Policy_SecurityContext* const context);

    const UA_SecurityPolicyAsymmetricModule asymmetricModule;
    const UA_SecurityPolicySymmetricModule symmetricModule;

    /**
     * The context of this security policy. Contains the server private key, certificate and other certificate information.
     * Needs to be initialized when adding the security policy to the server config.
     */
    UA_Policy_SecurityContext context;

    /**
     * Deletes the members (namely the context) of the security policy.
     * This method is only safe when passing the security policy this method is invoked on to itself.
     * The implementer of this method should somehow assert this.
     *
     * \param securityPolicy the security policy to delete the members of. Should only be the security policy the method is invoked on.
     *                       example: mySecurityPolicy.deleteMembers(&mySecurityPolicy);
     */
    UA_StatusCode (*const deleteMembers)(UA_SecurityPolicy* const securityPolicy);

    /**
     * Initializes the security policy.
     *
     * \param securityPolicy
     * \param logger
     * \param securityContext
     */
    UA_StatusCode (*const init)(UA_SecurityPolicy* const securityPolicy, UA_Logger logger);

    /* The channelContext prototype. This is copied by makeChannelContext */
    const UA_Channel_SecurityContext channelContextPrototype;

    /**
     * Makes a copy of the channelContext prototype of this security policy.
     * The copy needs to be initialized before use, its members deleted before desctruction and destroyed after use.
     *
     * \param securityPolicy Should only be the security policy the method is invoked on.
     *                       example: mySecurityPolicy.makeChannelContext(&mySecurityPolicy);
     * \param pp_SecurityContext a pointer to a pointer to the security context. The supplied
                                 pointer will be set to the memory where the SecurityContext is allocated
     */
    UA_StatusCode (*const makeChannelContext)(const UA_SecurityPolicy* const securityPolicy, UA_Channel_SecurityContext** const pp_SecurityContext);

    UA_Logger logger;
};

/**
 * \brief A helper struct that makes it easier passing around the available policies.
 */
typedef struct {
    /** The number of policies */
    size_t count;
    /** The policy array */
    UA_SecurityPolicy* policies;
} UA_SecurityPolicies;

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYPOLICY_H_