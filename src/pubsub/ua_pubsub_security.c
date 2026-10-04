/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "ua_pubsub.h"
#include "ua_util_internal.h"

static UA_Boolean
publisherIdEqual(const UA_PubSubReplayHistoryEntry *entry,
                 const UA_NetworkMessage *nm) {
    if(entry->publisherIdEnabled != nm->publisherIdEnabled)
        return false;
    if(!nm->publisherIdEnabled)
        return true;
    if(entry->publisherIdType != nm->publisherIdType)
        return false;

    switch(nm->publisherIdType) {
    case UA_PUBLISHERIDTYPE_BYTE:
        return entry->publisherId.byte == nm->publisherId.byte;
    case UA_PUBLISHERIDTYPE_UINT16:
        return entry->publisherId.uint16 == nm->publisherId.uint16;
    case UA_PUBLISHERIDTYPE_UINT32:
        return entry->publisherId.uint32 == nm->publisherId.uint32;
    case UA_PUBLISHERIDTYPE_UINT64:
        return entry->publisherId.uint64 == nm->publisherId.uint64;
    case UA_PUBLISHERIDTYPE_STRING:
        return UA_String_equal(&entry->publisherId.string,
                               &nm->publisherId.string);
    default:
        return false;
    }
}

static void
clearReplayHistoryEntry(UA_PubSubReplayHistoryEntry *entry) {
    if(entry->inUse && entry->publisherIdEnabled &&
       entry->publisherIdType == UA_PUBLISHERIDTYPE_STRING)
        UA_String_clear(&entry->publisherId.string);
    memset(entry, 0, sizeof(*entry));
}

/* Keep a bounded sequence history per token and PublisherId in each
 * ReaderGroup. */
UA_StatusCode
UA_ReaderGroup_checkReplay(UA_ReaderGroup *readerGroup,
                           const UA_NetworkMessage *nm) {
    if(!nm->securityEnabled)
        return UA_STATUSCODE_GOOD;
    if(nm->securityHeader.securityTokenId != readerGroup->securityTokenId ||
       nm->securityHeader.messageNonceSize < 8)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* The final four nonce bytes contain the little-endian sequence number. */
    const UA_Byte *nonce = &nm->securityHeader.messageNonce[4];
    UA_UInt32 sequenceNumber =
        (UA_UInt32)nonce[0] | ((UA_UInt32)nonce[1] << 8) |
        ((UA_UInt32)nonce[2] << 16) | ((UA_UInt32)nonce[3] << 24);

    UA_PubSubReplayHistoryEntry *entry = NULL;
    UA_PubSubReplayHistoryEntry *replacement = NULL;
    /* Find this publisher, or an empty/least-recently-used slot for it. */
    for(size_t i = 0; i < UA_PUBSUB_REPLAY_HISTORY_SIZE; i++) {
        UA_PubSubReplayHistoryEntry *candidate = &readerGroup->replayHistory[i];
        if(!candidate->inUse) {
            if(!replacement || replacement->inUse)
                replacement = candidate;
            continue;
        }
        if(candidate->securityTokenId == nm->securityHeader.securityTokenId &&
           publisherIdEqual(candidate, nm)) {
            entry = candidate;
            break;
        }
        if(!replacement || (replacement->inUse &&
                            candidate->useCounter < replacement->useCounter))
            replacement = candidate;
    }

    if(entry) {
        /* Serial arithmetic permits rollover while rejecting duplicates and
         * sequence numbers in the older half of the counter range. */
        UA_UInt32 distance = sequenceNumber - entry->sequenceNumber;
        if(distance == 0 || distance >= ((UA_UInt32)1 << 31))
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    } else {
        entry = replacement;
        clearReplayHistoryEntry(entry);
        entry->publisherIdEnabled = nm->publisherIdEnabled;
        entry->publisherIdType = nm->publisherIdType;
        if(nm->publisherIdEnabled) {
            if(nm->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
                UA_StatusCode res = UA_String_copy(&nm->publisherId.string,
                                                   &entry->publisherId.string);
                if(res != UA_STATUSCODE_GOOD)
                    return res;
            } else {
                entry->publisherId = nm->publisherId;
            }
        }
        entry->securityTokenId = nm->securityHeader.securityTokenId;
        entry->inUse = true;
    }

    entry->sequenceNumber = sequenceNumber;
    entry->useCounter = ++readerGroup->replayUseCounter;
    return UA_STATUSCODE_GOOD;
}

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
        *doDecrypt = true;
    } else if(!isEncrypted && !requiresEncryption) {
        *doDecrypt = false;
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

static UA_StatusCode
needsValidation(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doValidate) {
    UA_Boolean isSigned = networkMessage->securityHeader.networkMessageSigned;
    UA_Boolean requiresSignature = securityMode > UA_MESSAGESECURITYMODE_NONE;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(isSigned &&
       requiresSignature) {
        *doValidate = true;
    } else if(!isSigned && !requiresSignature) {
        *doValidate = false;
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
        if(buffer->length < sigSize) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                           "PubSub receive. Message too short for signature");
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        }
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

        /* Update replay history only after signature verification succeeds. */
        rv = UA_ReaderGroup_checkReplay(readerGroup, nm);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. duplicate or stale secured message");
    }

    return rv;
}
