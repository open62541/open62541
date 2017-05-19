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
    UA_StatusCode (*const init)(const UA_SecurityPolicy *securityPolicy,
                                const void *initData,
                                void **pp_contextData);

    UA_StatusCode (*const deleteMembers)(const UA_SecurityPolicy *securityPolicy,
                                         void *endpointContext);

    UA_StatusCode (*const setLocalPrivateKey)(const UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString *privateKey,
                                              void *endpointContext);

    UA_StatusCode (*const setLocalCertificate)(const UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString *certificate,
                                               void *endpointContext);

    const UA_ByteString * (*const getLocalCertificate)(const UA_SecurityPolicy *securityPolicy,
                                                       const void *endpointContext);

    UA_StatusCode (*const setCertificateTrustList)(const UA_SecurityPolicy *securityPolicy,
                                                   const UA_ByteString *trustList,
                                                   void *endpointContext);

    UA_StatusCode (*const setCertificateRevocationList)(const UA_SecurityPolicy *securityPolicy,
                                                        const UA_ByteString *revocationList,
                                                        void *endpointContext);

    /**
     * Gets the signature size that depends on the local private key.
     * This will return 0 as long as no remote certificate was set.
     */
    size_t(*const getLocalAsymSignatureSize)(const UA_SecurityPolicy *securityPolicy,
                                             const void *endpointContext);

    /**
    * \brief Compares the supplied certificate with the certificate in the endpoit context.
    *
    * \param securityPolicy contains the function pointers associated with the policy.
    * \param endpointContext the endpoint context data that contains the certificate to compare to.
    * \param certificateThumbprint the certificate thumbprint to compare to the one stored in the context.
    * \return if the thumbprints match UA_STATUSCODE_GOOD is returned. If they don't match
    *         or an errror occured an error code is returned.
    */
    UA_StatusCode(*const compareCertificateThumbprint)(const UA_SecurityPolicy *securityPolicy,
                                                       const void *endpointContext,
                                                       const UA_ByteString *certificateThumbprint);
};

struct UA_Channel_SecurityContext
{
    /**
     * \brief This method initializes a new context data object.
     * The caller needs to call deleteMembers on the recieved object to free allocated memory.
     *
     * Memory is only allocated if the function succeeds so there is no need to manually free
     * the memory pointed to by *pp_contextData or to call deleteMembers.
     * 
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param remoteCertificate the remote certificate contains the remote asymmetric key.
     *                          The certificate will be verified and then stored in the context
     *                          so that its details may be accessed.
     * \param contextData the initialized contextData that is passed to functions that work
     *                    on a context.
     */
    UA_StatusCode (*const init)(const UA_SecurityPolicy *securityPolicy,
                                const void *endpointContext,
                                const UA_ByteString *remoteCertificate,
                                void **pp_contextData);

    /**
     * \brief Deletes the members of the security context.
     *
     * \param securityPolicy contains the function pointers associated with the policy.
     * \param contextData the context to delete the members of.
     */
    UA_StatusCode (*const deleteMembers)(const UA_SecurityPolicy *securityPolicy,
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

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYCONTEXT_H_
