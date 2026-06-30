/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "open62541/transport_generated.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

UA_StatusCode
UA_SecureChannel_generateLocalNonce(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating new local nonce");

    /* Is the length of the previous nonce correct? */
    size_t nonceLength = sp->nonceLength;
    if(nonceLength == 0)
        return UA_STATUSCODE_GOOD;

    /* At least 32 byte */
    if(nonceLength < 32)
        nonceLength = 32;

    if(channel->localNonce.length != nonceLength) {
        UA_ByteString_clear(&channel->localNonce);
        UA_StatusCode res = UA_ByteString_allocBuffer(&channel->localNonce, nonceLength);
        UA_CHECK_STATUS(res, return res);
    }

    /* Generate the nonce */
    channel->localNonce.data[0] = 'e';
    channel->localNonce.data[1] = 'p';
    channel->localNonce.data[2] = 'h';
    return sp->generateNonce(sp, channel->channelContext, &channel->localNonce);
}

/* OPC UA Part 6 v1.05.07 §6.8.1 step 2 "Extract" — IKM chaining (enhanced
 * policies only). The IKM is XORed with the new shared secret on each renewal;
 * the accumulator is the curve's coordinate size (half the nonceLength, e.g. 32
 * bytes for P-256). It is carried through sp->generateKey without changing that
 * signature by prepending it to the `secret` ByteString: the backend's
 * DeriveKeys helper detects the prepend (key1 longer than the ephemeral public
 * key), XORs, and writes the chained IKM back into the slot. */
#define IKM_PREPEND_LENGTH(channel) \
    ((channel)->enhancedSecurity ? ((channel)->securityPolicy->nonceLength / 2) : 0)

/* Allocate and fill the prepend buffer [currentIKM | nonce] when
 * chaining is active; otherwise point *outInput at the original
 * nonce. The caller frees *outCombined with UA_ByteString_clear. */
static UA_StatusCode
prepareKeyInput(UA_SecureChannel *channel, const UA_ByteString *nonce,
                UA_ByteString *outInput, UA_ByteString *outCombined) {
    size_t ikmLen = IKM_PREPEND_LENGTH(channel);
    if(ikmLen == 0) {
        *outInput = *nonce;
        *outCombined = UA_BYTESTRING_NULL;
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res =
        UA_ByteString_allocBuffer(outCombined, ikmLen + nonce->length);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(channel->currentIKM.length == ikmLen)
        memcpy(outCombined->data, channel->currentIKM.data, ikmLen);
    else
        memset(outCombined->data, 0, ikmLen);
    memcpy(outCombined->data + ikmLen, nonce->data, nonce->length);
    *outInput = *outCombined;
    return UA_STATUSCODE_GOOD;
}

/* Pull the (possibly updated) IKM from the prepend slot and store
 * it as channel->currentIKM. Called only by the local-keys pass;
 * the remote-keys pass leaves the accumulator untouched. */
static UA_StatusCode
captureIKMSlot(UA_SecureChannel *channel, const UA_ByteString *combined) {
    size_t ikmLen = IKM_PREPEND_LENGTH(channel);
    if(ikmLen == 0)
        return UA_STATUSCODE_GOOD;
    UA_ByteString_clear(&channel->currentIKM);
    UA_ByteString ikmSlot = {ikmLen, combined->data};
    return UA_ByteString_copy(&ikmSlot, &channel->currentIKM);
}

UA_StatusCode
UA_SecureChannel_generateLocalKeys(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating new local keys");

    void *cc = channel->channelContext;
    const UA_SecurityPolicyEncryptionAlgorithm *ea = &sp->symEncryptionAlgorithm;

    /* Generate symmetric key buffer of the required length. The block size is
     * identical for local/remote. For AEAD ciphers the IV length differs from
     * the block size, so use getLocalIvLength when available. */
    UA_ByteString buf;
    size_t encrKL = ea->getLocalKeyLength(sp, cc);
    size_t encrBS = ea->getRemoteBlockSize(sp, cc);
    size_t ivLen = ea->getLocalIvLength ? ea->getLocalIvLength(sp, cc) : encrBS;
    size_t signKL = sp->symSignatureAlgorithm.getLocalKeyLength(sp, cc);
    if(ivLen + signKL + encrKL == 0)
        return UA_STATUSCODE_GOOD; /* No keys to generate */

    UA_StatusCode res = UA_ByteString_allocBuffer(&buf, ivLen + signKL + encrKL);
    UA_CHECK_STATUS(res, return res);
    UA_ByteString localSigningKey = {signKL, buf.data};
    UA_ByteString localEncryptingKey = {encrKL, &buf.data[signKL]};
    UA_ByteString localIv = {ivLen, &buf.data[signKL + encrKL]};

    /* TODO: Signal that no ECC salt is generated. Find a clean solution for this.  */
    buf.data[0] = 0x00;

    /* Build the IKM-prefixed `secret` input (the helper detects
     * the prepend via the length mismatch against the local
     * ephemeral public key). The `seed` arg is passed unchanged —
     * the helper uses it as the alternate ephemeral public key
     * candidate and as part of the salt, so the prepend must not
     * appear there. */
    UA_ByteString secretInput, seedInput = channel->localNonce;
    UA_ByteString secretCombined = UA_BYTESTRING_NULL;
    res = prepareKeyInput(channel, &channel->remoteNonce,
                         &secretInput, &secretCombined);
    UA_CHECK_STATUS(res, goto error);

    /* Generate key. The policy's wrapper detects the prepend (via the
     * length mismatch in key1 vs the local ephemeral public key) and
     * performs the IKM chaining, deriving against the *previous*
     * accumulator (channel->currentIKM). The new accumulator is NOT
     * promoted here: the local- and remote-keys passes of one OPN must
     * both derive against the same previous accumulator, so the
     * advance to IKM_n happens exactly once, in generateRemoteKeys
     * (the last of the two passes). */
    res = sp->generateKey(sp, cc, &secretInput, &seedInput, &buf);
    UA_CHECK_STATUS(res, goto error);

    /* Set the channel context */
    res |= sp->setLocalSymSigningKey(sp, cc, &localSigningKey);
    res |= sp->setLocalSymEncryptingKey(sp, cc, &localEncryptingKey);
    res |= sp->setLocalSymIv(sp, cc, &localIv);

 error:
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(sp->logger, channel,
                             "Could not generate local keys (%s)",
                             UA_StatusCode_name(res));
    }
    UA_ByteString_clear(&buf);
    UA_ByteString_clear(&secretCombined);
    return res;
}

UA_StatusCode
generateRemoteKeys(UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
    UA_LOG_DEBUG_CHANNEL(sp->logger, channel, "Generating new remote keys");

    void *cc = channel->channelContext;
    const UA_SecurityPolicyEncryptionAlgorithm *ea = &sp->symEncryptionAlgorithm;

    /* Generate symmetric key buffer of the required length. For AEAD ciphers
     * the IV length differs from the block size, so use getLocalIvLength when
     * available. */
    UA_ByteString buf;
    size_t encrKL = ea->getRemoteKeyLength(sp, cc);
    size_t encrBS = ea->getRemoteBlockSize(sp, cc);
    size_t ivLen = ea->getLocalIvLength ? ea->getLocalIvLength(sp, cc) : encrBS;
    size_t signKL = sp->symSignatureAlgorithm.getRemoteKeyLength(sp, cc);
    if(ivLen + signKL + encrKL == 0)
        return UA_STATUSCODE_GOOD; /* No keys to generate */

    UA_StatusCode res = UA_ByteString_allocBuffer(&buf, ivLen + signKL + encrKL);
    UA_CHECK_STATUS(res, return res);
    UA_ByteString remoteSigningKey = {signKL, buf.data};
    UA_ByteString remoteEncryptingKey = {encrKL, &buf.data[signKL]};
    UA_ByteString remoteIv = {ivLen, &buf.data[signKL + encrKL]};

    /* TODO: Signal that no ECC salt is generated. Find a clean solution for this.  */
    buf.data[0] = 0x00;

    /* Build the IKM-prefixed `secret` input. Both the local- and
     * remote-keys passes of one OPN derive against the *same* previous
     * accumulator (channel->currentIKM); generateLocalKeys deliberately
     * did not advance it. The `seed` arg is passed unchanged — the
     * helper uses it as the alternate ephemeral public key candidate
     * and as part of the salt, so the prepend must not appear there. */
    UA_ByteString secretInput, seedInput = channel->remoteNonce;
    UA_ByteString secretCombined = UA_BYTESTRING_NULL;
    res = prepareKeyInput(channel, &channel->localNonce,
                         &secretInput, &secretCombined);
    UA_CHECK_STATUS(res, goto error);

    /* Generate key. The policy's wrapper XORs the previous accumulator
     * with the new shared secret and writes the result (IKM_n) back
     * into secretCombined. */
    res = sp->generateKey(sp, cc, &secretInput, &seedInput, &buf);
    UA_CHECK_STATUS(res, goto error);

    /* This is the last derivation of the OPN: promote the just-updated
     * IKM slot into channel->currentIKM so the next renewal chains from
     * IKM_n. (On the first OPN currentIKM was empty, so IKM_0 == the raw
     * shared secret for both passes — matching the spec's "no chaining
     * on the first OpenSecureChannel".) */
    res = captureIKMSlot(channel, &secretCombined);
    if(res != UA_STATUSCODE_GOOD) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Set the channel context */
    res |= sp->setRemoteSymSigningKey(sp, cc, &remoteSigningKey);
    res |= sp->setRemoteSymEncryptingKey(sp, cc, &remoteEncryptingKey);
    res |= sp->setRemoteSymIv(sp, cc, &remoteIv);

 error:
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(sp->logger, channel,
                               "Could not generate remote keys (%s)",
                               UA_StatusCode_name(res));
    }
    UA_ByteString_clear(&buf);
    UA_ByteString_clear(&secretCombined);
    return res;
}

/* v1.05.07 channel-bound SignatureData (SecureChannelEnhancements): the
 * CreateSession / ActivateSession signatures are bound to the SecureChannel by
 * prepending the ChannelThumbprint and including certificate hashes (not the
 * raw certs). */

/* The certificate hash is provided by the crypto backend via the global
 * UA_SecurityPolicy_hashCertificate (the algorithm is derived from the policy
 * URI). Wrapped here so this file links without a crypto backend - the
 * builders below are only reached at runtime for enhanced-security policies,
 * which only exist when encryption is enabled. */
#ifdef UA_ENABLE_ENCRYPTION
static UA_StatusCode
hashCert(const UA_SecurityPolicy *sp, const UA_ByteString *cert,
         UA_ByteString *hash) {
    return UA_SecurityPolicy_hashCertificate(sp, cert, hash);
}
#else
static UA_StatusCode
hashCert(const UA_SecurityPolicy *sp, const UA_ByteString *cert,
         UA_ByteString *hash) {
    (void)sp; (void)cert; (void)hash;
    return UA_STATUSCODE_BADINTERNALERROR;
}
#endif

/* Assemble a channel-bound SignatureData (OPC UA Part 6 v1.05.07,
 * SecureChannelEnhancements). All three share the shape
 *   channelThumbprint | nonceA | H(cert_0) .. H(cert_n-1) | nonceB
 * and differ only in which certificate hashes appear and the nonce order:
 *
 *   CreateSession ServerSignature   (server signs, client verifies):
 *     TP | clientNonce | H(serverChannelCert) | H(clientChannelCert) | serverNonce
 *   ActivateSession ClientSignature (client signs, server verifies):
 *     TP | serverNonce | H(serverAppCert) | H(serverChannelCert) |
 *                        H(clientChannelCert) | clientNonce
 *   ActivateSession user-token sig  (client signs w/ user key, server verifies):
 *     TP | serverNonce | H(serverAppCert) | H(serverChannelCert) |
 *                        H(clientAppCert) | H(clientChannelCert) | clientNonce
 *
 * H() = the policy's curve hash of the leaf DER. Both peers must produce the
 * SAME bytes, so each side maps the logical certificate roles onto its own
 * local vs. remote view (the certs are channel-scoped: app == channel cert in
 * open62541, hence the duplicated hashes). The user/X.509-token certificate is
 * NOT part of the signed data (it is conveyed and validated separately).
 *
 *   logical cert role      server passes               client passes
 *   --------------------------------------------------------------------------
 *   server App / Channel   sp->localCertificate        channel->remoteCertificate
 *   client App / Channel   channel->remoteCertificate  sp->localCertificate
 *
 *   serverNonce = the session ServerNonce, clientNonce = the session
 *   ClientNonce (the same value on both sides). */
#define UA_MAX_SIGDATA_CERTS 4
static UA_StatusCode
buildChannelBoundSignatureData(const UA_SecureChannel *channel,
                               const UA_ByteString *firstNonce,
                               const UA_ByteString *const *certs, size_t certCount,
                               const UA_ByteString *lastNonce, UA_ByteString *out) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!sp || !channel->enhancedSecurity || certCount > UA_MAX_SIGDATA_CERTS ||
       channel->channelThumbprint.length == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Hash the certificates. The inputs may be DER chains (e.g. the OPN
     * SenderCertificate stored in remoteCertificate), so hash only the leaf -
     * independent of how a given crypto backend's hashCert handles a chain. */
    UA_ByteString hashes[UA_MAX_SIGDATA_CERTS];
    for(size_t i = 0; i < UA_MAX_SIGDATA_CERTS; i++)
        hashes[i] = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    size_t hashesLen = 0;
    for(size_t i = 0; i < certCount && res == UA_STATUSCODE_GOOD; i++) {
        UA_ByteString leaf = getLeafCertificate(*certs[i]);
        res = hashCert(sp, &leaf, &hashes[i]);
        hashesLen += hashes[i].length;
    }
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* channelThumbprint | firstNonce | hashes | lastNonce */
    const UA_ByteString *tp = &channel->channelThumbprint;
    res = UA_ByteString_allocBuffer(out, tp->length + firstNonce->length +
                                    hashesLen + lastNonce->length);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    size_t o = 0;
    memcpy(out->data + o, tp->data, tp->length); o += tp->length;
    memcpy(out->data + o, firstNonce->data, firstNonce->length); o += firstNonce->length;
    for(size_t i = 0; i < certCount; i++) {
        memcpy(out->data + o, hashes[i].data, hashes[i].length);
        o += hashes[i].length;
    }
    memcpy(out->data + o, lastNonce->data, lastNonce->length);

 cleanup:
    for(size_t i = 0; i < certCount; i++)
        UA_ByteString_clear(&hashes[i]);
    return res;
}

/* CreateSession ServerSignature (see the layout table above). */
UA_StatusCode
UA_SecureChannel_buildCreateSessionSignatureData(
    const UA_SecureChannel *channel, const UA_ByteString *clientNonce,
    const UA_ByteString *serverNonce, const UA_ByteString *serverChannelCert,
    const UA_ByteString *clientChannelCert, UA_ByteString *out) {
    const UA_ByteString *certs[] = {serverChannelCert, clientChannelCert};
    return buildChannelBoundSignatureData(channel, clientNonce, certs, 2,
                                          serverNonce, out);
}

/* ActivateSession ClientSignature (see the layout table above). */
UA_StatusCode
UA_SecureChannel_buildActivateSessionSignatureData(
    const UA_SecureChannel *channel, const UA_ByteString *serverNonce,
    const UA_ByteString *clientNonce, const UA_ByteString *serverAppCert,
    const UA_ByteString *serverChannelCert, const UA_ByteString *clientChannelCert,
    UA_ByteString *out) {
    const UA_ByteString *certs[] = {serverAppCert, serverChannelCert, clientChannelCert};
    return buildChannelBoundSignatureData(channel, serverNonce, certs, 3,
                                          clientNonce, out);
}

/* ActivateSession X.509 user-token signature (see the layout table above). */
UA_StatusCode
UA_SecureChannel_buildUserTokenSignatureData(
    const UA_SecureChannel *channel, const UA_ByteString *serverNonce,
    const UA_ByteString *clientNonce, const UA_ByteString *serverAppCert,
    const UA_ByteString *serverChannelCert, const UA_ByteString *clientAppCert,
    const UA_ByteString *clientChannelCert, UA_ByteString *out) {
    const UA_ByteString *certs[] = {serverAppCert, serverChannelCert,
                                    clientAppCert, clientChannelCert};
    return buildChannelBoundSignatureData(channel, serverNonce, certs, 4,
                                          clientNonce, out);
}

/***************************/
/* Send Asymmetric Message */
/***************************/

/* The length of the static header content */
#define UA_SECURECHANNEL_ASYMMETRIC_SECURITYHEADER_FIXED_LENGTH 12

size_t
calculateAsymAlgSecurityHeaderLength(const UA_SecureChannel *channel) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

    size_t asymHeaderLength = UA_SECURECHANNEL_ASYMMETRIC_SECURITYHEADER_FIXED_LENGTH +
                              sp->policyUri.length;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return asymHeaderLength;

    /* OPN is always encrypted even if the mode is sign only */
    asymHeaderLength += 20; /* Thumbprints are always 20 byte long */
    asymHeaderLength += sp->localCertificate.length;
    return asymHeaderLength;
}

UA_StatusCode
prependHeadersAsym(UA_SecureChannel *const channel, UA_Byte *header_pos,
                   const UA_Byte *buf_end, size_t totalLength,
                   size_t securityHeaderLength, UA_UInt32 requestId,
                   size_t *const encryptedLength) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

    void *cc = channel->channelContext;

    *encryptedLength = totalLength;
    if(channel->securityMode != UA_MESSAGESECURITYMODE_NONE) {
        size_t dataToEncryptLength = totalLength -
            (UA_SECURECHANNEL_CHANNELHEADER_LENGTH + securityHeaderLength);
        size_t plainTextBlockSize = sp->asymEncryptionAlgorithm.
            getRemotePlainTextBlockSize(sp, cc);
        size_t encryptedBlockSize = sp->asymEncryptionAlgorithm.
            getRemoteBlockSize(sp, cc);

        /* Padding always fills up the last block */
        UA_assert(plainTextBlockSize > 0);
        UA_assert(dataToEncryptLength % plainTextBlockSize == 0);
        size_t blocks = dataToEncryptLength / plainTextBlockSize;
        *encryptedLength = totalLength + blocks * (encryptedBlockSize - plainTextBlockSize);
    }

    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    messageHeader.messageSize = (UA_UInt32)*encryptedLength;
    UA_UInt32 secureChannelId = channel->securityToken.channelId;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_encodeBinaryInternal(&messageHeader,
                                   &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                   &header_pos, &buf_end, NULL, NULL, NULL);
    res |= UA_UInt32_encodeBinary(&secureChannelId, &header_pos, buf_end);
    UA_CHECK_STATUS(res, return res);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = sp->policyUri;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        asymHeader.senderCertificate = sp->localCertificate;
        asymHeader.receiverCertificateThumbprint.length = 20;
        asymHeader.receiverCertificateThumbprint.data = channel->remoteCertificateThumbprint;
    }
    res = UA_encodeBinaryInternal(&asymHeader,
                                  &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER],
                                  &header_pos, &buf_end, NULL, NULL, NULL);
    UA_CHECK_STATUS(res, return res);

    UA_SequenceHeader seqHeader;
    seqHeader.requestId = requestId;
    seqHeader.sequenceNumber = UA_SecureChannel_nextSequenceNumber(channel);
    res = UA_encodeBinaryInternal(&seqHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER],
                                  &header_pos, &buf_end, NULL, NULL, NULL);
    return res;
}

void
hideBytesAsym(const UA_SecureChannel *channel, UA_Byte **buf_start,
              const UA_Byte **buf_end) {
    /* Set buf_start to the beginning of the encrypted body */
    *buf_start += UA_SECURECHANNEL_CHANNELHEADER_LENGTH;
    *buf_start += calculateAsymAlgSecurityHeaderLength(channel);

    /* Hide only the SequenceHeader for None  */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE) {
        *buf_start += UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH;
        return;
    }

    /* The max plaintext length depends on the number of encrypted blocks that
     * can fit into the remaining chunk */
    void *cc = channel->channelContext;
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    size_t plainTextBlockSize =
        sp->asymEncryptionAlgorithm.getRemotePlainTextBlockSize(sp, cc);
    size_t encryptedBlockSize =
        sp->asymEncryptionAlgorithm.getRemoteBlockSize(sp, cc);

    size_t max_encrypted = (size_t)(*buf_end - *buf_start);
    UA_assert(encryptedBlockSize > 0);
    size_t max_blocks = max_encrypted / encryptedBlockSize;
    size_t max_plaintext = max_blocks * plainTextBlockSize;

    /* Reserve plaintext length for the SequenceHeader and Footer.
     * But don't reserve for the the padding itself -- which can be zero. */
    max_plaintext -= UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH;
    max_plaintext -= sp->asymSignatureAlgorithm.getLocalSignatureSize(sp, cc);
    UA_Boolean extraPadding =
        (sp->asymEncryptionAlgorithm.getRemoteKeyLength(sp, cc) > 2048);
    max_plaintext -= (UA_LIKELY(!extraPadding)) ? 1u : 2u;

    /* Adjust the buffer */
    *buf_end = *buf_start + max_plaintext;
    *buf_start += UA_SECURECHANNEL_SEQUENCEHEADER_LENGTH;
}

/* Assumes that pos can be advanced to the end of the current block */
void
padChunk(UA_SecureChannel *channel,
         const UA_SecurityPolicySignatureAlgorithm *sa,
         const UA_SecurityPolicyEncryptionAlgorithm *ea,
         const UA_Byte *start, UA_Byte **pos) {
    UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;

    const size_t bytesToWrite = (uintptr_t)*pos - (uintptr_t)start;

    size_t signatureSize = sa->getLocalSignatureSize(sp, cc);
    size_t plainTextBlockSize = ea->getRemotePlainTextBlockSize(sp, cc);
    UA_Boolean extraPadding = (ea->getRemoteKeyLength(sp, cc) > 2048);
    size_t paddingBytes = (UA_LIKELY(!extraPadding)) ? 1u : 2u;

    UA_assert(plainTextBlockSize > 0);
    size_t lastBlock = ((bytesToWrite + signatureSize + paddingBytes) % plainTextBlockSize);
    size_t paddingLength = (lastBlock != 0) ? plainTextBlockSize - lastBlock : 0;

    UA_assert((bytesToWrite + signatureSize +
               paddingBytes + paddingLength) % plainTextBlockSize == 0);

    UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                         "Add %lu bytes of padding plus %lu padding size bytes",
                         (long unsigned int)paddingLength,
                         (long unsigned int)paddingBytes);

    /* Write the padding. This is <= because the paddingSize byte also has to be
     * written */
    UA_Byte paddingByte = (UA_Byte)paddingLength;
    for(size_t i = 0; i <= paddingLength; ++i) {
        **pos = paddingByte;
        ++*pos;
    }

    /* Write the extra padding byte if required */
    if(extraPadding) {
        **pos = (UA_Byte)(paddingLength >> 8u);
        ++*pos;
    }
}

UA_StatusCode
signAndEncryptAsym(UA_SecureChannel *channel, size_t preSignLength,
                   UA_ByteString *buf, size_t securityHeaderLength,
                   size_t totalLength) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;

    /* OPC UA Part 6 v1.05.07 §6.7.5 "ChannelThumbprint" (enhanced policies,
     * FIRST OPN only — an empty channelThumbprint identifies the first OPN; NOT
     * on renewals. The OPN response signature is extended with the first OPN
     * request signature: the client (request side) stores its just-computed
     * signature for the later response verify; the server (response side)
     * appends the request signature captured by decryptAndVerifyChunk to the
     * data being signed. */
    UA_Boolean firstOPN = (channel->enhancedSecurity &&
                           channel->channelThumbprint.length == 0);
    UA_ByteString *appendSig = NULL;
    if(firstOPN && channel->firstRequestSignature.length > 0)
        appendSig = &channel->firstRequestSignature; /* server: extend signed data */

    size_t sigsize = sp->asymSignatureAlgorithm.getLocalSignatureSize(sp, cc);

    /* Prepare the data to sign. If we need to append the request
     * signature, build a contiguous buffer [body | requestSig] and sign
     * that. Otherwise sign the body in-place. */
    UA_ByteString signature = {sigsize, buf->data + preSignLength};
    UA_StatusCode retval;

    if(appendSig) {
        size_t bodyLen = preSignLength;
        size_t extLen = bodyLen + appendSig->length;
        UA_Byte *ext = (UA_Byte*)UA_malloc(extLen);
        if(!ext)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(ext, buf->data, bodyLen);
        memcpy(ext + bodyLen, appendSig->data, appendSig->length);
        UA_ByteString dataToSignExt = {extLen, ext};
        retval = sp->asymSignatureAlgorithm.sign(sp, cc, &dataToSignExt, &signature);
        UA_free(ext);
    } else {
        const UA_ByteString dataToSign = {preSignLength, buf->data};
        retval = sp->asymSignatureAlgorithm.sign(sp, cc, &dataToSign, &signature);
        /* First-OPN client side: remember the just-computed request
         * signature. The verify path on the client will use it to
         * extend the response body. */
        if(retval == UA_STATUSCODE_GOOD && firstOPN && signature.length > 0) {
            UA_StatusCode clip =
                UA_ByteString_copy(&signature, &channel->firstRequestSignature);
            if(clip != UA_STATUSCODE_GOOD)
                retval = clip;
        }
    }

    UA_CHECK_STATUS(retval, return retval);

    /* Server, first OPN response (appendSig != NULL): the just-computed
     * signature is the ChannelThumbprint - capture it (preserved across renewals
     * to bind the session SignatureData) and release the consumed request
     * signature. (The client stored its request signature above; that copy is
     * released later by the OPN-response verify.) */
    if(appendSig != NULL) {
        UA_StatusCode tp = UA_ByteString_copy(&signature, &channel->channelThumbprint);
        UA_CHECK_STATUS(tp, return tp);
        UA_ByteString_clear(&channel->firstRequestSignature);
    }

    /* Specification part 6, 6.7.4: The OpenSecureChannel Messages are
     * signed and encrypted if the SecurityMode is not None (even if the
     * SecurityMode is SignOnly). */
    size_t unencrypted_length =
        UA_SECURECHANNEL_CHANNELHEADER_LENGTH + securityHeaderLength;
    UA_ByteString dataToEncrypt =
        {totalLength - unencrypted_length, &buf->data[unencrypted_length]};
    return sp->asymEncryptionAlgorithm.encrypt(sp, cc, &dataToEncrypt);
}

/**************************/
/* Send Symmetric Message */
/**************************/

UA_StatusCode
signAndEncryptSym(UA_MessageContext *messageContext,
                  size_t preSigLength, size_t totalLength) {
    const UA_SecureChannel *channel = messageContext->channel;
    if(channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;

    /* For AEAD policies (ChaCha20-Poly1305), set the message security
     * parameters for nonce masking. Then let the encrypt callback handle
     * both authentication and encryption. */
    if(UA_SecurityPolicy_isAead(sp)) {
        /* Set AEAD parameters: tokenId, previous seqNo, AAD */
        if(sp->setMessageSecurityParameters) {
            UA_ByteString aad;
            aad.data = messageContext->messageBuffer.data;
            aad.length = UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
                         UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH;
            /* The AEAD nonce is masked with the PREVIOUS sequence number: the
             * receiver masks with its receiveSequenceNumber, which is still the
             * prior chunk's number at decrypt time (incremented only after
             * decryption). sendSequenceNumber is the post-increment counter, so
             * the value just emitted is (sendSequenceNumber - 1) and the
             * previous one is (sendSequenceNumber - 2). ECC_AEAD always uses the
             * non-legacy counter (first emitted number is 0). */
            UA_UInt32 prevSeqNo = channel->sendSequenceNumber >= 2 ?
                (UA_UInt32)(channel->sendSequenceNumber - 2) : 0;
            UA_StatusCode res = sp->setMessageSecurityParameters(
                sp, cc, channel->securityToken.tokenId, prevSeqNo, &aad);
            UA_CHECK_STATUS(res, return res);
        }

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
            /* Full AEAD: encrypt + compute auth tag. Data includes
             * the 16-byte tag space at the end. */
            UA_ByteString dataToProcess;
            dataToProcess.data = messageContext->messageBuffer.data +
                UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
                UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH;
            dataToProcess.length = totalLength -
                (UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
                 UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH);
            return sp->symEncryptionAlgorithm.encrypt(sp, cc, &dataToProcess);
        }

        /* Sign-only: Compute Poly1305 auth tag */
        UA_ByteString dataToSign = messageContext->messageBuffer;
        dataToSign.length = preSigLength;
        UA_ByteString signature;
        signature.length =
            sp->symSignatureAlgorithm.getLocalSignatureSize(sp, cc);
        signature.data = messageContext->buf_pos;
        return sp->symSignatureAlgorithm.
            sign(sp, cc, &dataToSign, &signature);
    }

    /* Sign */
    UA_ByteString dataToSign = messageContext->messageBuffer;
    dataToSign.length = preSigLength;
    UA_ByteString signature;
    signature.length =
        sp->symSignatureAlgorithm.getLocalSignatureSize(sp, cc);
    signature.data = messageContext->buf_pos;
    UA_StatusCode res = sp->symSignatureAlgorithm.
        sign(sp, cc, &dataToSign, &signature);
    UA_CHECK_STATUS(res, return res);

    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    /* Encrypt */
    UA_ByteString dataToEncrypt;
    dataToEncrypt.data = messageContext->messageBuffer.data +
        UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
        UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH;
    dataToEncrypt.length = totalLength -
        (UA_SECURECHANNEL_CHANNELHEADER_LENGTH +
         UA_SECURECHANNEL_SYMMETRIC_SECURITYHEADER_LENGTH);
    return sp->symEncryptionAlgorithm.encrypt(sp, cc, &dataToEncrypt);
}

void
setBufPos(UA_MessageContext *mc) {
    /* Forward the data pointer so that the payload is encoded after the message
     * header. This has to be a symmetric message because OPN (with asymmetric
     * encryption) does not support chunking. */
    mc->buf_pos = &mc->messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH];
    mc->buf_end = &mc->messageBuffer.data[mc->messageBuffer.length];

    if(mc->channel->securityMode == UA_MESSAGESECURITYMODE_NONE)
        return;

    const UA_SecureChannel *channel = mc->channel;
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;

    /* For AEAD (ChaCha20-Poly1305): no padding, no block alignment.
     * Only reserve space for the authentication tag (signature size). */
    if(UA_SecurityPolicy_isAead(sp)) {
        size_t sigsize =
            sp->symSignatureAlgorithm.getLocalSignatureSize(sp, cc);
        mc->buf_end -= sigsize;
        UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                             "Prepare an AEAD symmetric message buffer of length %lu "
                             "with a usable maximum payload length of %lu",
                             (long unsigned)mc->messageBuffer.length,
                             (long unsigned)((uintptr_t)mc->buf_end -
                                             (uintptr_t)mc->messageBuffer.data));
        return;
    }

    size_t sigsize =
        sp->symSignatureAlgorithm.getLocalSignatureSize(sp, cc);
    size_t plainBlockSize =
        sp->symEncryptionAlgorithm.getRemotePlainTextBlockSize(sp, cc);

    /* Assuming that for symmetric encryption the plainTextBlockSize ==
     * cypherTextBlockSize. For symmetric encryption the remote/local block
     * sizes are identical. */
    UA_assert(sp->symEncryptionAlgorithm.getRemoteBlockSize(sp, cc) == plainBlockSize);

    /* Leave enough space for the signature and padding */
    mc->buf_end -= sigsize;
    UA_assert(plainBlockSize > 0);
    mc->buf_end -= mc->messageBuffer.length % plainBlockSize;

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Reserve space for the padding bytes */
        UA_Boolean extraPadding =
            (sp->symEncryptionAlgorithm.getRemoteKeyLength(sp, cc) > 2048);
        mc->buf_end -= (UA_LIKELY(!extraPadding)) ? 1 : 2;
    }

    UA_LOG_TRACE_CHANNEL(sp->logger, channel,
                         "Prepare a symmetric message buffer of length %lu "
                         "with a usable maximum payload length of %lu",
                         (long unsigned)mc->messageBuffer.length,
                         (long unsigned)((uintptr_t)mc->buf_end -
                                         (uintptr_t)mc->messageBuffer.data));
}

/****************************/
/* Process a received Chunk */
/****************************/

static size_t
decodePadding(const UA_SecureChannel *channel,
              const UA_SecurityPolicyEncryptionAlgorithm *encryptionAlgorithm,
              const UA_ByteString *chunk, size_t sigsize) {
    /* Read the byte with the padding size */
    size_t paddingSize = chunk->data[chunk->length - sigsize - 1];

    /* Extra padding size */
    if(encryptionAlgorithm->getLocalKeyLength(channel->securityPolicy,
                                              channel->channelContext) > 2048) {
        paddingSize <<= 8u;
        paddingSize += chunk->data[chunk->length - sigsize - 2];
        paddingSize += 1; /* Extra padding byte itself */
    }

    /* Add one since the paddingSize byte itself needs to be removed as well */
    return paddingSize + 1;
}

/* Sets the payload to a pointer inside the chunk buffer. Returns the requestId
 * and the sequenceNumber */
UA_StatusCode
decryptAndVerifyChunk(UA_SecureChannel *channel,
                      const UA_SecurityPolicySignatureAlgorithm *signatureAlgorithm,
                      const UA_SecurityPolicyEncryptionAlgorithm *encryptionAlgorithm,
                      UA_MessageType messageType, UA_ByteString *chunk,
                      size_t offset) {
    UA_SecurityPolicy *sp = channel->securityPolicy;
    void *cc = channel->channelContext;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* AEAD path (ChaCha20-Poly1305): the decrypt callback handles both
     * decryption and authentication tag verification in one step. */
    if(UA_SecurityPolicy_isAead(sp) &&
       messageType != UA_MESSAGETYPE_OPN) {

        /* Set AEAD parameters before decrypt/verify. The masked nonce is keyed
         * on the TokenId of *this* message, which is taken from the chunk's
         * symmetric SecurityHeader (right after the 12-byte channel header) and
         * not from channel->securityToken: during a token rollover the peer may
         * still secure messages with the old token (Part 4 §5.5.2) while the
         * channel already holds the new token. Using the wrong TokenId here
         * makes the AEAD tag verification fail. */
        if(sp->setMessageSecurityParameters) {
            UA_ByteString aad = {offset, chunk->data};
            size_t tokenOffset = UA_SECURECHANNEL_CHANNELHEADER_LENGTH;
            UA_UInt32 msgTokenId = channel->securityToken.tokenId;
            if(offset >= UA_SECURECHANNEL_MESSAGE_MIN_LENGTH)
                UA_UInt32_decodeBinary(chunk, &tokenOffset, &msgTokenId);
            res = sp->setMessageSecurityParameters(
                sp, cc, msgTokenId,
                channel->receiveSequenceNumber, &aad);
            UA_CHECK_STATUS(res, return res);
        }

        if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
            /* Full AEAD decrypt + verify. The decrypt callback processes
             * the data including the 16-byte auth tag at the end. On
             * success, it reduces cipher.length by the tag size. */
            UA_ByteString cipher = {chunk->length - offset, chunk->data + offset};
            res = encryptionAlgorithm->decrypt(sp, cc, &cipher);
            UA_CHECK_STATUS(res,
                UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                                       "AEAD decryption/verification failed");
                return res);
            chunk->length = cipher.length + offset;
        } else if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN) {
            /* Sign-only: verify the auth tag without decryption */
            size_t sigsize = signatureAlgorithm->getRemoteSignatureSize(sp, cc);
            UA_CHECK(sigsize < chunk->length,
                     return UA_STATUSCODE_BADSECURITYCHECKSFAILED);
            const UA_ByteString content = {chunk->length - sigsize, chunk->data};
            const UA_ByteString sig = {sigsize, chunk->data + chunk->length - sigsize};
            res = signatureAlgorithm->verify(sp, cc, &content, &sig);
            UA_CHECK_STATUS(res,
                UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                                       "AEAD signature verification failed");
                return res);
            chunk->length -= sigsize;
        }

        /* Verify the content length */
        UA_CHECK(offset + 9 < chunk->length,
                 UA_LOG_ERROR_CHANNEL(sp->logger, channel,
                                      "AEAD message too short");
                 return UA_STATUSCODE_BADSECURITYCHECKSFAILED);
        return UA_STATUSCODE_GOOD;
    }

    /* Decrypt the chunk */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ||
       messageType == UA_MESSAGETYPE_OPN) {
        UA_ByteString cipher = {chunk->length - offset, chunk->data + offset};
        res = encryptionAlgorithm->decrypt(sp, cc, &cipher);
        UA_CHECK_STATUS(res, return res);
        chunk->length = cipher.length + offset;
    }

    /* Does the message have a signature? */
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       messageType != UA_MESSAGETYPE_OPN)
        return UA_STATUSCODE_GOOD;

    /* Verify the chunk signature */
    UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Verifying chunk signature");
    size_t sigsize = signatureAlgorithm->getRemoteSignatureSize(sp, cc);
    UA_CHECK(sigsize < chunk->length, return UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    const UA_ByteString content = {chunk->length - sigsize, chunk->data};
    const UA_ByteString sig = {sigsize, chunk->data + chunk->length - sigsize};

    /* OPC UA Part 6 v1.05.07 §6.7.5 "ChannelThumbprint" (only for the first
     * OPN exchange and only for SecurityPolicies with
     * secureChannelEnhancements = true). For the *first* OPN only:
     *
     *   Server side (verify an incoming OPN request): the request
     *   signature is unknown to the server until it has been
     *   verified. Capture the bytes now (BEFORE stripping) so the
     *   sign path of the OPN response can append them.
     *
     *   Client side (verify the OPN response): the client's own
     *   request signature was stored in firstRequestSignature when
     *   the request was sent. Extend the verify content with it.
     *
     * For OPN *renewals* the firstRequestSignature is empty (it was
     * cleared by the first exchange) and the normal v1.05.06 verify
     * path is used. */
    UA_ByteString verifyContent = content;
    UA_Byte *extendedBuf = NULL;
    UA_Boolean capturedIncoming = false;
    /* First-OPN only (channelThumbprint empty). On renewals neither capture nor
     * extend - the v1.05.06 verify path is used. */
    UA_Boolean firstOPN = (channel->enhancedSecurity &&
                           messageType == UA_MESSAGETYPE_OPN &&
                           channel->channelThumbprint.length == 0);
    if(firstOPN) {
        if(channel->firstRequestSignature.length == 0) {
            /* First-OPN, receiving side: capture the incoming signature.
             * The sign path of the response will consume it. */
            UA_StatusCode clip = UA_ByteString_copy(&sig, &channel->firstRequestSignature);
            UA_CHECK_STATUS(clip, return clip);
            capturedIncoming = true;
        } else {
            /* Verifying the OPN response: extend the content. */
            extendedBuf = (UA_Byte*)UA_malloc(
                content.length + channel->firstRequestSignature.length);
            if(!extendedBuf) {
                UA_ByteString_clear(&channel->firstRequestSignature);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(extendedBuf, content.data, content.length);
            memcpy(extendedBuf + content.length,
                   channel->firstRequestSignature.data,
                   channel->firstRequestSignature.length);
            verifyContent.data = extendedBuf;
            verifyContent.length =
                content.length + channel->firstRequestSignature.length;
        }
    }

    res = signatureAlgorithm->verify(sp, cc, &verifyContent, &sig);
    UA_free(extendedBuf);

    /* Single-use: if we just consumed the stored signature (client side
     * verifying the response), clear it. If we just captured the
     * incoming signature (server side verifying the request), keep
     * it — the sign path of the response will use and then clear it. */
    if(firstOPN && !capturedIncoming) {
        UA_ByteString_clear(&channel->firstRequestSignature);
    }

    UA_CHECK_STATUS(res, UA_LOG_WARNING_CHANNEL(sp->logger, channel,
                                                "Could not verify the signature");
                    return res);

    /* OPC UA Part 6 v1.05.07 ChannelThumbprint: on the client, the
     * verified first-OPN *response* signature (sig) is the
     * ChannelThumbprint. capturedIncoming is false only on the
     * client response-verify path. Capture once, preserve across
     * renewals; binds the session SignatureData to this channel. */
    if(channel->enhancedSecurity && messageType == UA_MESSAGETYPE_OPN &&
       !capturedIncoming && channel->channelThumbprint.length == 0) {
        UA_StatusCode tp = UA_ByteString_copy(&sig, &channel->channelThumbprint);
        UA_CHECK_STATUS(tp, return tp);
    }

    /* Compute the padding if the payload is encrypted (not ECC policy) */
    size_t padSize = 0;
    if((messageType != UA_MESSAGETYPE_OPN &&
        channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) ||
       (messageType == UA_MESSAGETYPE_OPN &&
        sp->policyType == UA_SECURITYPOLICYTYPE_RSA)) {
        padSize = decodePadding(channel, encryptionAlgorithm, chunk, sigsize);
        UA_LOG_TRACE_CHANNEL(sp->logger, channel, "Calculated padding size to be %lu",
                             (long unsigned)padSize);
    }

    /* Verify the content length. The encrypted payload has to be at least 9
     * bytes long: 8 byte for the SequenceHeader and one byte for the actual
     * message */
    UA_CHECK(offset + padSize + sigsize + 9 < chunk->length,
             UA_LOG_ERROR_CHANNEL(sp->logger, channel, "Impossible padding value");
             return UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* Hide the signature and padding */
    chunk->length -= (sigsize + padSize);
    return UA_STATUSCODE_GOOD;
}

/* The certificate in the header is verified via the configured PKI plugin as
 * certificateVerification.verifyCertificate(...). We cannot do it here because
 * the client/server context is needed. */
UA_StatusCode
checkAsymHeader(UA_SecureChannel *channel,
                const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(!UA_String_equal(&sp->policyUri, &asymHeader->securityPolicyUri))
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    return sp->compareCertThumbprint(sp, &asymHeader->receiverCertificateThumbprint);
}

UA_StatusCode
checkSymHeader(UA_SecureChannel *channel, const UA_UInt32 tokenId,
               UA_DateTime nowMonotonic) {
    UA_SecurityPolicy *sp = channel->securityPolicy;
    (void)sp;

    /* If no match, try to revolve to the next token after a
     * RenewSecureChannel */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ChannelSecurityToken *token = &channel->securityToken;
    switch(channel->renewState) {
    case UA_SECURECHANNELRENEWSTATE_NORMAL:
    case UA_SECURECHANNELRENEWSTATE_SENT:
    default:
        break;

    case UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER:
        /* Old token still in use */
        if(tokenId == channel->securityToken.tokenId)
            break;

        /* Not the new token */
        UA_CHECK(tokenId == channel->altSecurityToken.tokenId,
                 UA_LOG_ERROR_CHANNEL(sp->logger, channel, "Unknown SecurityToken");
                 return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN);

        /* Roll over to the new token, generate new local and remote keys */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        channel->securityToken = channel->altSecurityToken;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        retval |= UA_SecureChannel_generateLocalKeys(channel);
        retval |= generateRemoteKeys(channel);
        UA_CHECK_STATUS(retval, return retval);
        break;

    case UA_SECURECHANNELRENEWSTATE_NEWTOKEN_CLIENT:
        /* The server is still using the old token. That's okay. */
        if(tokenId == channel->altSecurityToken.tokenId) {
            token = &channel->altSecurityToken;
            break;
        }

        /* Not the new token */
        UA_CHECK(tokenId == channel->securityToken.tokenId,
                 UA_LOG_ERROR_CHANNEL(sp->logger, channel, "Unknown SecurityToken");
                 return UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN);

        /* The remote server uses the new token for the first time. Delete the
         * old token and roll the remote key over. The local key already uses
         * the nonce pair from the last OPN exchange. */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        retval = generateRemoteKeys(channel);
        UA_CHECK_STATUS(retval, return retval);
    }

    UA_DateTime timeout = token->createdAt + (token->revisedLifetime * UA_DATETIME_MSEC);
    if(channel->state == UA_SECURECHANNELSTATE_OPEN && timeout < nowMonotonic) {
        UA_LOG_ERROR_CHANNEL(sp->logger, channel, "SecurityToken timed out");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_TIMEOUT);
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;
    }

    return UA_STATUSCODE_GOOD;
}

UA_Boolean
UA_SecureChannel_checkTimeout(UA_SecureChannel *channel, UA_DateTime nowMonotonic) {
    /* Compute the timeout date of the SecurityToken */
    UA_DateTime timeout = channel->securityToken.createdAt +
        (UA_DateTime)(channel->securityToken.revisedLifetime * UA_DATETIME_MSEC);

    /* The token has timed out. Try to do the token revolving now instead of
     * shutting the channel down.
     *
     * Part 4, 5.5.2 says: Servers shall use the existing SecurityToken to
     * secure outgoing Messages until the SecurityToken expires or the
     * Server receives a Message secured with a new SecurityToken.*/
    if(timeout < nowMonotonic &&
       channel->renewState == UA_SECURECHANNELRENEWSTATE_NEWTOKEN_SERVER) {
        /* Revolve the token manually. This is otherwise done in checkSymHeader. */
        channel->renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
        channel->securityToken = channel->altSecurityToken;
        UA_ChannelSecurityToken_init(&channel->altSecurityToken);
        UA_SecureChannel_generateLocalKeys(channel);
        generateRemoteKeys(channel);

        /* Use the timeout of the new SecurityToken */
        timeout = channel->securityToken.createdAt +
            (UA_DateTime)(channel->securityToken.revisedLifetime * UA_DATETIME_MSEC);
    }

    return (timeout < nowMonotonic);
}
