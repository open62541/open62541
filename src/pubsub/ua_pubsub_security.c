/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "ua_pubsub_manager.h"
#include "ua_util_internal.h"

static
UA_StatusCode
needsDecryption(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doDecrypt) {

    UA_Boolean isEncrypted = networkMessage->securityHeader.networkMessageEncrypted;
    UA_Boolean requiresEncryption = securityMode > UA_MESSAGESECURITYMODE_SIGN;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(isEncrypted && requiresEncryption) {
        *doDecrypt = UA_TRUE;
    } else if(!isEncrypted && !requiresEncryption) {
        *doDecrypt = UA_FALSE;
    } else {
        if(isEncrypted) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is encrypted but ReaderGroup does not expect encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not encrypted but ReaderGroup requires encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

static
UA_StatusCode
needsValidation(const UA_Logger *logger,
                    const UA_NetworkMessage *networkMessage,
                    const UA_MessageSecurityMode securityMode,
                    UA_Boolean *doValidate) {

    UA_Boolean isSigned = networkMessage->securityHeader.networkMessageSigned;
    UA_Boolean requiresSignature = securityMode > UA_MESSAGESECURITYMODE_NONE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(isSigned &&
       requiresSignature) {
        *doValidate = UA_TRUE;
    } else if(!isSigned && !requiresSignature) {
        *doValidate = UA_FALSE;
    } else {

        if(isSigned) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is signed but ReaderGroup does not expect signatures");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not signed but ReaderGroup requires signature");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

UA_StatusCode
verifyAndDecrypt(const UA_Logger *logger, UA_ByteString *buffer,
                 const size_t *currentPosition, const UA_NetworkMessage *nm,
                 UA_Boolean doValidate, UA_Boolean doDecrypt, void *channelContext,
                 UA_PubSubSecurityPolicy *securityPolicy) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;

    if(doValidate) {
        size_t sigSize = securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(channelContext);
        UA_ByteString toBeVerified = {buffer->length - sigSize, buffer->data};
        UA_ByteString signature = {sigSize, buffer->data + buffer->length - sigSize};

        rv = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            verify(channelContext, &toBeVerified, &signature);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Signature invalid");

        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "PubSub receive. Signature valid");
        buffer->length -= sigSize;
    }

    if(doDecrypt) {
        const UA_ByteString nonce = {
            (size_t)nm->securityHeader.messageNonceSize,
            (UA_Byte*)(uintptr_t)nm->securityHeader.messageNonce
        };
        rv = securityPolicy->setMessageNonce(channelContext, &nonce);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Nonce set");

        UA_ByteString toBeDecrypted = {buffer->length - *currentPosition,
                                       buffer->data + *currentPosition};
        rv = securityPolicy->symmetricModule.cryptoModule
                 .encryptionAlgorithm.decrypt(channelContext, &toBeDecrypted);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Decryption");
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
verifyAndDecryptNetworkMessage(const UA_Logger *logger, UA_ByteString *buffer,
                               size_t *currentPosition, UA_NetworkMessage *nm,
                               UA_ReaderGroup *readerGroup) {
    UA_MessageSecurityMode securityMode = readerGroup->config.securityMode;
    UA_Boolean doValidate = false;
    UA_Boolean doDecrypt = false;

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    rv = needsValidation(logger, nm, securityMode, &doValidate);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Validation security mode error");

    rv = needsDecryption(logger, nm, securityMode, &doDecrypt);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Decryption security mode error");

    if(doValidate || doDecrypt) {
        void *channelContext = readerGroup->securityPolicyContext;
        UA_PubSubSecurityPolicy *securityPolicy = readerGroup->config.securityPolicy;
        UA_CHECK_MEM_ERROR(channelContext, return UA_STATUSCODE_BADINVALIDARGUMENT,
                           logger, UA_LOGCATEGORY_SERVER,
                           "PubSub receive. securityPolicyContext must be initialized "
                           "when security mode is enabled to sign and/or encrypt");
        UA_CHECK_MEM_ERROR(securityPolicy, return UA_STATUSCODE_BADINVALIDARGUMENT,
                           logger, UA_LOGCATEGORY_SERVER,
                           "PubSub receive. securityPolicy must be set when security mode"
                           "is enabled to sign and/or encrypt");

        rv = verifyAndDecrypt(logger, buffer, currentPosition, nm,
                              doValidate, doDecrypt, channelContext, securityPolicy);

        UA_CHECK_STATUS_ERROR(rv, return rv, logger, UA_LOGCATEGORY_SERVER,
                              "PubSub receive. verify and decrypt failed");
    }

    return rv;
}
