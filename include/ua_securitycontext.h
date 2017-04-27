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

#ifndef UA_SECURITYCONTEXT_H_
#define UA_SECURITYCONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_log.h"
#include "ua_securitypolicy_fwd.h"
#include "ua_securitycontext_fwd.h"

struct UA_Endpoint_SecurityContext
{
    UA_StatusCode (*const init)(const UA_SecurityPolicy *const securityPolicy,
                                const void *const initData,
                                void **const pp_contextData);

    UA_StatusCode (*const deleteMembers)(const UA_SecurityPolicy *const securityPolicy,
                                         void *const endpointContext);

    UA_StatusCode (*const setServerPrivateKey)(const UA_SecurityPolicy *const securityPolicy,
                                               const UA_ByteString *const privateKey,
                                               void *const endpointContext);

    UA_StatusCode (*const setCertificateTrustList)(const UA_SecurityPolicy *const securityPolicy,
                                                   const UA_ByteString *const trustList,
                                                   void *const endpointContext);

    UA_StatusCode (*const setCertificateRevocationList)(const UA_SecurityPolicy *const securityPolicy,
                                                        const UA_ByteString *const revocationList,
                                                        void *const endpointContext);

    /**
     * Gets the signature size that depends on the local private key.
     * This will return 0 as long as no remote certificate was set.
     */
    size_t(*const getLocalAsymSignatureSize)(const UA_SecurityPolicy *const securityPolicy,
                                             const void *const endpointContext);
};

struct UA_Channel_SecurityContext
{
    /**
     * \brief This method initializes a new context data object.
     * The caller needs to call deleteMembers on the recieved object to free allocated memory.
     * 
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param logger the logger this SecurityContext may use.
     * \param contextData the initialized contextData that is passed to functions that work
     *                    on a context.
     */
    UA_StatusCode (*const init)(const UA_SecurityPolicy *const securityPolicy,
                                void **const pp_contextData);

    /**
     * \brief Deletes the members of the security context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to delete the members of.
     */
    UA_StatusCode (*const deleteMembers)(const UA_SecurityPolicy *const securityPolicy,
                                         void *const contextData);

    /**
     * \brief Sets the local encrypting key in the supplied context.
     * 
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the local encrypting key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymEncryptingKey)(const UA_SecurityPolicy *const securityPolicy,
                                                    const UA_ByteString *const key,
                                                    void *const contextData);

    /**
     * \brief Sets the local signing key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the local signing key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymSigningKey)(const UA_SecurityPolicy *const securityPolicy,
                                                 const UA_ByteString *const key,
                                                 void *const contextData);

    /**
     * \brief Sets the local initialization vector in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param iv the local initialization vector to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setLocalSymIv)(const UA_SecurityPolicy *const securityPolicy,
                                         const UA_ByteString *const iv,
                                         void *const contextData);
    /**
     * \brief Sets the remote encrypting key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the remote encrypting key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setRemoteSymEncryptingKey)(const UA_SecurityPolicy *const securityPolicy,
                                                     const UA_ByteString *const key,
                                                     void *const contextData);

    /**
     * \brief Sets the remote signing key in the supplied context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param key the remote signing key to store in the context.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const setRemoteSymSigningKey)(const UA_SecurityPolicy *const securityPolicy,
                                                  const UA_ByteString *const key,
                                                  void *const contextData);

    /**
    * \brief Sets the remote initialization vector in the supplied context.
    *
    * \param securityPolicy contains the function pointers associated with the policy.
    * \param iv the remote initialization vector to store in the context.
    * \param contextData the context to work on.
    */
    UA_StatusCode (*const setRemoteSymIv)(const UA_SecurityPolicy *const securityPolicy,
                                          const UA_ByteString *const iv,
                                          void *const contextData);
    /**
     * \brief Parses a given certificate to extract the remote public key from it.
     * Fails, if the certificate is invalid or expired.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param remoteCertificate the remote certificate to extract the client public key from.
     * \param contextData the context to work on.
     */
    UA_StatusCode (*const parseRemoteCertificate)(const UA_SecurityPolicy *const securityPolicy,
                                                  const UA_ByteString *const remoteCertificate,
                                                  void *const contextData);

    /**
     * \brief Gets the signature size that depends on the remote public key.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to retrieve data from.
     * \return the size of the remote asymmetric signature. Returns 0 if no remote certificate
     *                     was set previousely.
     */
    size_t (*const getRemoteAsymSignatureSize)(const UA_SecurityPolicy *const securityPolicy,
                                               const void *const contextData);

    /**
     * \brief Gets the plaintext block size that depends on the remote public key.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to retrieve data from.
     * \return the size of the plain text block size when encrypting with the remote public key.
     *         Returns 0 as long as no remote certificate was set previousely.
     */
    size_t (*const getRemoteAsymPlainTextBlockSize)(const UA_SecurityPolicy *const securityPolicy,
                                                    const void *const contextData);

    /**
     * Gets the number of bytes that are needed by the encryption function in addition to the length of the plaintext message.
     * This is needed, since most rsa encryption methods have their own padding mechanism included. This makes the encrypted
     * message larger than the plainText, so we need to have enough room in the buffer for the overhead.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the retrieve data from.
     * \param maxEncryptionLength the maximum number of bytes that the data to encrypt can be.
     */
    size_t (*const getRemoteAsymEncryptionBufferLengthOverhead)(const UA_SecurityPolicy *const securityPolicy,
                                                                const void *const contextData,
                                                                const size_t maxEncryptionLength);
};

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYCONTEXT_H_
