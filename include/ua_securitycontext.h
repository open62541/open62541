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

struct UA_Policy_SecurityContext
{
    UA_StatusCode (*const init)(UA_Policy_SecurityContext *const securityContext,
                                const UA_SecurityPolicy *const securityPolicy,
                                UA_Logger logger,
                                void *const initData);

    UA_StatusCode (*const deleteMembers)(UA_Policy_SecurityContext *const securityContext);

    UA_StatusCode (*const setServerPrivateKey)(UA_Policy_SecurityContext *const securityContext,
                                               const UA_ByteString *const privateKey);

    UA_StatusCode (*const setCertificateTrustList)(UA_Policy_SecurityContext *const securityContext,
                                                   const UA_ByteString *const trustList);

    UA_StatusCode (*const setCertificateRevocationList)(UA_Policy_SecurityContext *const securityContext,
                                                        const UA_ByteString *const revocationList);

    /**
     * Gets the signature size that depends on the local private key.
     * This will return 0 as long as no remote certificate was set.
     */
    size_t(*const getLocalAsymSignatureSize)(const UA_Policy_SecurityContext *const securityContext);

    void* data;

    UA_Logger logger;

    /** Pointer to securityPolicy this context is linked with */
    const UA_SecurityPolicy *securityPolicy;
};

struct UA_Channel_SecurityContext
{
    /**
     * This method initializes the context data object. Needs to be called before using
     * the context and after copying it from the security policy.
     * 
     * \param securityContext the SecurityContext to initialize. Should always be the context this is called on.
     *                        example: myCtx.init(&myCtx);
     * \param securityPolicy the security policy the context is linked to.
     * \param logger the logger this SecurityContext may use.
     */
    UA_StatusCode (*const init)(UA_Channel_SecurityContext* const securityContext,
                                const UA_SecurityPolicy *const securityPolicy,
                                UA_Logger logger);

    /**
     * Deletes the members of the security context.
     *
     * \param securityContext the context to delete the members of. Should always be the context this is called on.
     *                        example: myCtx.free(&myCtx);
     */
    UA_StatusCode (*const deleteMembers)(UA_Channel_SecurityContext* const securityContext);

    /**
     * Sets the local encrypting key in the supplied context.
     * 
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param key the local encrypting key to store in the context.
     */
    UA_StatusCode (*const setLocalSymEncryptingKey)(UA_Channel_SecurityContext* const securityContext,
                                                 const UA_ByteString* const key);

    /**
     * Sets the local signing key in the supplied context.
     *
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param key the local signing key to store in the context.
     */
    UA_StatusCode (*const setLocalSymSigningKey)(UA_Channel_SecurityContext* const securityContext,
                                              const UA_ByteString* const key);

    /**
     * Sets the local initialization vector in the supplied context.
     *
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param iv the local initialization vector to store in the context.
     */
    UA_StatusCode (*const setLocalSymIv)(UA_Channel_SecurityContext* const securityContext,
                                      const UA_ByteString* const iv);
    /**
     * Sets the remote encrypting key in the supplied context.
     *
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param key the remote encrypting key to store in the context.
     */
    UA_StatusCode (*const setRemoteSymEncryptingKey)(UA_Channel_SecurityContext* const securityContext,
                                                  const UA_ByteString* const key);

    /**
     * Sets the remote signing key in the supplied context.
     *
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param key the remote signing key to store in the context.
     */
    UA_StatusCode (*const setRemoteSymSigningKey)(UA_Channel_SecurityContext *const securityContext,
                                               const UA_ByteString *const key);

    /**
    * Sets the remote initialization vector in the supplied context.
    *
    * \param securityContext the context to work on. Should always be the context this is called on.
    * \param iv the remote initialization vector to store in the context.
    */
    UA_StatusCode (*const setRemoteSymIv)(UA_Channel_SecurityContext *const securityContext,
                                       const UA_ByteString *const iv);
    /**
     * Parses a given certificate to extract the remote public key from it.
     * Fails, if the certificate is invalid or expired.
     *
     * \param securityContext the context to work on. Should always be the context this is called on.
     * \param remoteCertificate the remote certificate to extract the client public key from.
     */
    UA_StatusCode (*const parseRemoteCertificate)(UA_Channel_SecurityContext *const securityContext,
                                                  const UA_ByteString *const remoteCertificate);

    /**
     * Gets the signature size that depends on the remote public key.
     * This will return 0 as long as no remote certificate was set.
     */
    size_t (*const getRemoteAsymSignatureSize)(const UA_Channel_SecurityContext *const securityContext);

    /**
     * Gets the plaintext block size that depends on the remote public key.
     * This will return 0 as long as no remote certificate was set.
     */
    size_t (*const getRemoteAsymPlainTextBlockSize)(const UA_Channel_SecurityContext *const securityContext);

    /**
     * Gets the number of bytes that are needed by the encryption function in addition to the length of the plaintext message.
     * This is needed, since most rsa encryption methods have their own padding mechanism included. This makes the encrypted
     * message larger than the plainText, so we need to have enough room in the buffer for the overhead.
     *
     * \param securityContext the context to work on
     * \param maxEncryptionLength the maximum number of bytes that the data to encrypt can be.
     */
    size_t (*const getRemoteAsymEncryptionBufferLengthOverhead)(const UA_Channel_SecurityContext *const securityContext,
                                                                const size_t maxEncryptionLength);

    UA_Logger logger;

    void *data;

    /** Pointer to securityPolicy this context is linked with */
    const UA_SecurityPolicy *securityPolicy;
};

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYCONTEXT_H_
