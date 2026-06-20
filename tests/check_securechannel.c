/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/transport_generated.h>
#include <open62541/types_generated.h>
#include <open62541/server_config_default.h>

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "util/ua_util_internal.h"

#include "testing_networklayers.h"
#include "testing_policy.h"

#include <stdlib.h>
#include <check.h>

#define UA_BYTESTRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)(s)}

// Some default testing sizes. Can be overwritten in testing functions.
#define DEFAULT_SYM_ENCRYPTION_BLOCK_SIZE 2
#define DEFAULT_SYM_SIGNING_KEY_LENGTH 3
#define DEFAULT_SYM_ENCRYPTION_KEY_LENGTH 5
#define DEFAULT_ASYM_REMOTE_SIGNATURE_SIZE 7
#define DEFAULT_ASYM_LOCAL_SIGNATURE_SIZE 11
#define DEFAULT_SYM_SIGNATURE_SIZE 13
#define DEFAULT_ASYM_REMOTE_PLAINTEXT_BLOCKSIZE 256
#define DEFAULT_ASYM_REMOTE_BLOCKSIZE 256

static UA_SecureChannel testChannel;
static UA_ByteString dummyCertificate =
    UA_BYTESTRING_STATIC("DUMMY CERTIFICATE DUMMY CERTIFICATE DUMMY CERTIFICATE");
static UA_SecurityPolicy dummyPolicy;
static UA_ConnectionManager *testCM;

static funcs_called fCalled;
static key_sizes keySizes;

static void
setup_secureChannel(void) {
    TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled, &keySizes);
    UA_SecureChannel_init(&testChannel);
    testChannel.config = UA_ConnectionConfig_default;
    UA_SecureChannel_setSecurityPolicy(&testChannel, &dummyPolicy, &dummyCertificate);

    testCM = TestConnectionManager_new("tcp", NULL);
    testChannel.connectionManager = testCM;
    testChannel.state = UA_SECURECHANNELSTATE_OPEN;
}

static void
teardown_secureChannel(void) {
    UA_SecureChannel_clear(&testChannel);
    dummyPolicy.clear(&dummyPolicy);
    UA_ConnectionManager *cm = testCM;
    testCM = NULL;
    cm->eventSource.free(&cm->eventSource);
}

static void
setup_funcs_called(void) {
    memset(&fCalled, 0, sizeof(struct funcs_called));
}

static void
teardown_funcs_called(void) {
    memset(&fCalled, 0, sizeof(struct funcs_called));
}

static void
setup_key_sizes(void) {
    memset(&keySizes, 0, sizeof(struct key_sizes));

    keySizes.sym_sig_keyLen = DEFAULT_SYM_SIGNING_KEY_LENGTH;
    keySizes.sym_enc_blockSize = DEFAULT_SYM_ENCRYPTION_BLOCK_SIZE;
    keySizes.sym_enc_keyLen = DEFAULT_SYM_ENCRYPTION_KEY_LENGTH;
    keySizes.sym_sig_size = DEFAULT_SYM_SIGNATURE_SIZE;

    keySizes.asym_lcl_sig_size = DEFAULT_ASYM_LOCAL_SIGNATURE_SIZE;
    keySizes.asym_rmt_sig_size = DEFAULT_ASYM_REMOTE_SIGNATURE_SIZE;

    keySizes.asym_rmt_ptext_blocksize = DEFAULT_ASYM_REMOTE_PLAINTEXT_BLOCKSIZE;
    keySizes.asym_rmt_blocksize = DEFAULT_ASYM_REMOTE_BLOCKSIZE;
    keySizes.asym_rmt_enc_key_size = 2048;
    keySizes.asym_lcl_enc_key_size = 1024;
}

static void
teardown_key_sizes(void) {
    memset(&keySizes, 0, sizeof(struct key_sizes));
}

START_TEST(SecureChannel_initAndDelete) {
    TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled, &keySizes);
    UA_StatusCode retval;

    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.config = UA_ConnectionConfig_default;
    retval = UA_SecureChannel_setSecurityPolicy(&channel, &dummyPolicy, &dummyCertificate);

    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode to be good");
    ck_assert_msg(channel.state == UA_SECURECHANNELSTATE_CLOSED,
                  "Expected state to be closed");
    ck_assert_msg(fCalled.newContext, "Expected newContext to have been called");
    ck_assert_msg(fCalled.makeCertificateThumbprint,
                  "Expected makeCertificateThumbprint to have been called");
    ck_assert_msg(channel.securityPolicy == &dummyPolicy, "SecurityPolicy not set correctly");

    UA_SecureChannel_clear(&channel);
    ck_assert_msg(fCalled.deleteContext, "Expected deleteContext to have been called");

    dummyPolicy.clear(&dummyPolicy);
}END_TEST

static void
createDummyResponse(UA_OpenSecureChannelResponse *response) {
    UA_OpenSecureChannelResponse_init(response);
    memset(response, 0, sizeof(UA_OpenSecureChannelResponse));
}

START_TEST(SecureChannel_sendAsymmetricOPNMessage_invalidParameters) {
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, 42, NULL,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendOPN(&testChannel, 42, &dummyResponse, NULL);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

}END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeNone) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);
    testChannel.securityMode = UA_MESSAGESECURITYMODE_NONE;
    testChannel.securityPolicy->policyType = UA_SECURITYPOLICYTYPE_NONE;

    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, 42, &dummyResponse,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");
    ck_assert_msg(!fCalled.asym_enc, "Message encryption was called but should not have been");
    ck_assert_msg(!fCalled.asym_sign, "Message signing was called but should not have been");
}
END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeSign) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);
    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, 42, &dummyResponse,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");
    ck_assert_msg(fCalled.asym_enc, "Expected message to have been encrypted but it was not");
    ck_assert_msg(fCalled.asym_sign, "Expected message to have been signed but it was not");
}END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeSignAndEncrypt) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, 42, &dummyResponse,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");
    ck_assert_msg(fCalled.asym_enc, "Expected message to have been encrypted but it was not");
    ck_assert_msg(fCalled.asym_sign, "Expected message to have been signed but it was not");
}END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_sentDataIsValid) {
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    /* Enable encryption for the SecureChannel */
    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_UInt32 requestId = UA_UInt32_random();

    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, requestId, &dummyResponse,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");

    const UA_ByteString *sent = TestConnectionManager_getLastSent(testCM);
    size_t offset = 0;
    UA_TcpMessageHeader header;
    retval = UA_decodeBinaryInternal(sent, &offset, &header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(sent, &offset, &secureChannelId);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    retval = UA_decodeBinaryInternal(sent, &offset, &asymSecurityHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(UA_ByteString_equal(&testChannel.securityPolicy->policyUri,
                                      &asymSecurityHeader.securityPolicyUri),
                  "Expected securityPolicyUri to be equal to the one used by the secureChannel");

    ck_assert_msg(UA_ByteString_equal(&dummyCertificate, &asymSecurityHeader.senderCertificate),
                  "Expected the certificate to be equal to the one used  by the secureChannel");

    UA_ByteString thumbPrint = {20, testChannel.remoteCertificateThumbprint};
    ck_assert_msg(UA_ByteString_equal(&thumbPrint,
                                      &asymSecurityHeader.receiverCertificateThumbprint),
                  "Expected receiverCertificateThumbprint to be equal to the one set "
                  "in the secureChannel");

    /* Dummy encryption */
    for(size_t i = offset; i < header.messageSize; ++i) {
        sent->data[i] = (UA_Byte)((sent->data[i] - 1) % (UA_BYTE_MAX + 1));
    }

    UA_SequenceHeader sequenceHeader;
    retval = UA_decodeBinaryInternal(sent, &offset, &sequenceHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %u but was %u",
                  requestId,
                  sequenceHeader.requestId);

    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(sent, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    retval = UA_decodeBinaryInternal(sent, &offset, &sentResponse, &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

    UA_Byte paddingByte = sent->data[offset];
    size_t paddingSize = (size_t)paddingByte;

    for(size_t i = 0; i <= paddingSize; ++i) {
        ck_assert_msg(sent->data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      (int)i, paddingByte, sent->data[offset + i]);
    }

    ck_assert_msg(sent->data[offset + paddingSize + 1] == '*', "Expected first byte of signature");

    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymSecurityHeader);
    UA_SequenceHeader_clear(&sequenceHeader);
    UA_OpenSecureChannelResponse_clear(&sentResponse);
} END_TEST

START_TEST(Securechannel_sendAsymmetricOPNMessage_extraPaddingPresentWhenKeyLargerThan2048Bits) {
    keySizes.asym_rmt_enc_key_size = 4096;
    keySizes.asym_rmt_blocksize = 4096;
    keySizes.asym_rmt_ptext_blocksize = 4096;

    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_UInt32 requestId = UA_UInt32_random();

    UA_StatusCode retval =
        UA_SecureChannel_sendOPN(&testChannel, requestId, &dummyResponse,
                                 &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");

    const UA_ByteString *sent = TestConnectionManager_getLastSent(testCM);
    size_t offset = 0;
    UA_TcpMessageHeader header;
    retval = UA_decodeBinaryInternal(sent, &offset, &header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(sent, &offset, &secureChannelId);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    retval = UA_decodeBinaryInternal(sent, &offset, &asymSecurityHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(UA_ByteString_equal(&dummyCertificate, &asymSecurityHeader.senderCertificate),
                  "Expected the certificate to be equal to the one used  by the secureChannel");
    ck_assert_msg(UA_ByteString_equal(&testChannel.securityPolicy->policyUri,
                                      &asymSecurityHeader.securityPolicyUri),
                  "Expected securityPolicyUri to be equal to the one used by the secureChannel");
    UA_ByteString thumbPrint = {20, testChannel.remoteCertificateThumbprint};
    ck_assert_msg(UA_ByteString_equal(&thumbPrint,
                                      &asymSecurityHeader.receiverCertificateThumbprint),
                  "Expected receiverCertificateThumbprint to be equal to the one set "
                  "in the secureChannel");

    for(size_t i = offset; i < header.messageSize; ++i) {
        sent->data[i] = (UA_Byte)((sent->data[i] - 1) % (UA_BYTE_MAX + 1));
    }

    UA_SequenceHeader sequenceHeader;
    retval = UA_decodeBinaryInternal(sent, &offset, &sequenceHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %u but was %u",
                  requestId, sequenceHeader.requestId);

    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(sent, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    retval = UA_decodeBinaryInternal(sent, &offset, &sentResponse, &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

    UA_Byte paddingByte = sent->data[sent->length - keySizes.asym_lcl_sig_size - 1];
    size_t paddingSize = (size_t)paddingByte;
    UA_Boolean extraPadding =
        (testChannel.securityPolicy->asymEncryptionAlgorithm.getRemoteKeyLength(
            testChannel.securityPolicy, testChannel.channelContext) > 2048);
    UA_Byte extraPaddingByte = 0;
    if(extraPadding) {
        extraPaddingByte = paddingByte;
        paddingByte = sent->data[sent->length - keySizes.asym_lcl_sig_size - 2];
        paddingSize = ((size_t)extraPaddingByte << 8u) + paddingByte;
        paddingSize += 1;
    }

    for(size_t i = 0; i < paddingSize; ++i) {
        ck_assert_msg(sent->data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      (int)i, paddingByte, sent->data[offset + i]);
    }

    if(extraPadding) {
        ck_assert_msg(sent->data[offset + paddingSize] == extraPaddingByte,
                      "Expected extra padding byte to be %i but got %i",
                      extraPaddingByte, sent->data[offset + paddingSize]);
    }
    ck_assert_msg(sent->data[offset + paddingSize + 1] == '*',
                  "Expected first byte 42 of signature but got %i",
                  sent->data[offset + paddingSize + 1]);

    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymSecurityHeader);
    UA_SequenceHeader_clear(&sequenceHeader);
    UA_OpenSecureChannelResponse_clear(&sentResponse);
}END_TEST

START_TEST(SecureChannel_sendSymmetricMessage) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    UA_StatusCode retval = UA_SecureChannel_sendMSG(&testChannel, 42,
                                                    &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    // TODO: expand test
}
END_TEST

START_TEST(SecureChannel_sendSymmetricMessage_modeNone) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    testChannel.securityMode = UA_MESSAGESECURITYMODE_NONE;

    UA_StatusCode retval = UA_SecureChannel_sendMSG(&testChannel, 42,
                                                    &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_msg(!fCalled.sym_sign, "Expected message to not have been signed");
    ck_assert_msg(!fCalled.sym_enc, "Expected message to not have been encrypted");
} END_TEST

START_TEST(SecureChannel_sendSymmetricMessage_modeSign) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_StatusCode retval = UA_SecureChannel_sendMSG(&testChannel, 42,
                                                    &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_msg(fCalled.sym_sign, "Expected message to have been signed");
    ck_assert_msg(!fCalled.sym_enc, "Expected message to not have been encrypted");
} END_TEST

START_TEST(SecureChannel_sendSymmetricMessage_modeSignAndEncrypt)
{
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_StatusCode retval = UA_SecureChannel_sendMSG(&testChannel, 42,
                                                    &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_msg(fCalled.sym_sign, "Expected message to have been signed");
    ck_assert_msg(fCalled.sym_enc, "Expected message to have been encrypted");
} END_TEST

START_TEST(SecureChannel_sendSymmetricMessage_invalidParameters) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    UA_StatusCode retval = UA_SecureChannel_sendMSG(NULL, 42,
                                                    &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendMSG(&testChannel, 42, NULL, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendMSG(&testChannel, 42, &dummyMessage, NULL);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");
} END_TEST

static UA_StatusCode
UA_SecureChannel_processBuffer(UA_SecureChannel *channel, int *chunks_processed,
                               const UA_ByteString buffer) {
    UA_StatusCode res = UA_SecureChannel_loadBuffer(channel, buffer);
    while(UA_LIKELY(res == UA_STATUSCODE_GOOD)) {
        UA_MessageType messageType;
        UA_UInt32 requestId = 0;
        UA_ByteString payload = UA_BYTESTRING_NULL;
        UA_Boolean copied = false;
        res = UA_SecureChannel_getCompleteMessage(channel, &messageType, &requestId,
                                                  &payload, &copied, UA_DateTime_nowMonotonic());
        if(res != UA_STATUSCODE_GOOD || payload.length == 0)
            break;
        ck_assert_uint_ne(payload.length, 0);
        ck_assert_ptr_ne(payload.data, NULL);
        ++*chunks_processed;
        if(copied)
            UA_ByteString_clear(&payload);
    }
    res |= UA_SecureChannel_persistBuffer(channel);
    return res;
}

START_TEST(SecureChannel_assemblePartialChunks) {
    int chunks_processed = 0;
    UA_ByteString buffer = UA_BYTESTRING_NULL;

    buffer.data = (UA_Byte *)"HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff";
    buffer.length = 32;

    UA_StatusCode retval =
        UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 1);

    buffer.length = 16;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 1);

    buffer.data = &buffer.data[16];
    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 2);

    buffer.data = (UA_Byte *)"HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff"
                             "HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff"
                             "HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff";
    buffer.length = 48;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 3);

    buffer.data = &buffer.data[48];
    buffer.length = 32;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 4);

    buffer.data = &buffer.data[32];
    buffer.length = 16;
    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 5);
} END_TEST

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) && !defined(LIBRESSL_VERSION_NUMBER)
/* OPC UA Part 6 v1.05.07 §6.8.1 step 2 "Extract" — IKM chaining on
 * SecureChannel renewal. This exercises the OpenSSL helper directly
 * to verify the chained-IKM semantics end-to-end. */

#include "crypto/openssl/securitypolicy_common.h"
#include <openssl/evp.h>
#include <openssl/ec.h>

/* Generate a fresh P-256 ephemeral key pair. The public key is
 * returned in uncompressed X9.62 form (65 bytes, leading 0x04). */
static EVP_PKEY *
makeP256Key(void) {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if(!pctx) return NULL;
    if(EVP_PKEY_keygen_init(pctx) != 1 ||
       EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) != 1) {
        EVP_PKEY_CTX_free(pctx);
        return NULL;
    }
    EVP_PKEY *kp = NULL;
    if(EVP_PKEY_keygen(pctx, &kp) != 1) {
        EVP_PKEY_CTX_free(pctx);
        return NULL;
    }
    EVP_PKEY_CTX_free(pctx);
    return kp;
}

/* Extract the local public key as the 64-byte x||y form used by
 * UA_OpenSSL_ECC_DeriveKeys. */
static UA_StatusCode
exportP256PublicXY(EVP_PKEY *kp, UA_Byte *out64) {
    UA_Byte *enc = NULL;
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    size_t encLen = EVP_PKEY_get1_encoded_public_key(kp, &enc);
#else
    size_t encLen = EVP_PKEY_get1_tls_encodedpoint(kp, &enc);
#endif
    if(encLen != 65 || enc[0] != 0x04) {
        OPENSSL_free(enc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    memcpy(out64, enc + 1, 64);
    OPENSSL_free(enc);
    return UA_STATUSCODE_GOOD;
}

START_TEST(SecureChannel_IKMChaining_prependChainsOnRenewal) {
    /* Two ephemeral key pairs (P-256). The helper detects the
     * prepend by checking if `key1` is longer than the expected
     * ephemeral public key, XORs the prepend with the raw shared
     * secret, and writes the chained IKM back to the prepend slot.
     * We exercise the helper twice with different prepends on the
     * same ephemeral key pair, and verify that the XOR difference
     * of the two resulting slot values equals the XOR difference
     * of the two prepends (the shared secret is identical in both
     * calls, so it cancels out: (P1^SS) XOR (P2^SS) = P1 XOR P2). */
    EVP_PKEY *localKp = makeP256Key();
    EVP_PKEY *remoteKp = makeP256Key();
    ck_assert_ptr_ne(localKp, NULL);
    ck_assert_ptr_ne(remoteKp, NULL);

    UA_Byte localPubXY[64];
    UA_Byte remotePubXY[64];
    ck_assert_int_eq(exportP256PublicXY(localKp, localPubXY), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(exportP256PublicXY(remoteKp, remotePubXY), UA_STATUSCODE_GOOD);

    /* Build two distinct 32-byte prepends. */
    UA_Byte prepend1[32];
    UA_Byte prepend2[32];
    for(int i = 0; i < 32; i++) {
        prepend1[i] = (UA_Byte)(0xA0 + i);
        prepend2[i] = (UA_Byte)(0x40 + i);
    }
    /* Slot buffers = [prepend | local ephemeral public key]. The
     * helper identifies which arg is local by matching against the
     * localEphemeralKeyPair's public key. So we put localPubXY as
     * the suffix of the prepend, and the helper will then take
     * key2 (== remotePubXY) as the remote ephemeral public key for
     * the ECDH computation. */
    UA_Byte slot1[32 + 64];
    UA_Byte slot2[32 + 64];
    memcpy(slot1, prepend1, 32);
    memcpy(slot1 + 32, localPubXY, 64);
    memcpy(slot2, prepend2, 32);
    memcpy(slot2 + 32, localPubXY, 64);

    UA_ByteString secret1 = {32 + 64, slot1};
    UA_ByteString secret2 = {32 + 64, slot2};
    /* seed (= key2) is the remote ephemeral public key — it must be
     * the un-prefixed form because the helper uses it both for the
     * salt and for ECDH when the prepend suffix matches local. */
    UA_ByteString seed1 = {64, remotePubXY};
    UA_ByteString seed2 = {64, remotePubXY};

    UA_ByteString out;
    ck_assert_int_eq(UA_ByteString_allocBuffer(&out, 32), UA_STATUSCODE_GOOD);

    /* First call: prepend1 -> chained IKM written back to slot1. */
    ck_assert_int_eq(UA_OpenSSL_ECC_DeriveKeys(
                         EC_curve_nist2nid("P-256"), "SHA256",
                         UA_APPLICATIONTYPE_CLIENT, localKp,
                         &secret1, &seed1, &out),
                     UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
    ck_assert_int_eq(UA_ByteString_allocBuffer(&out, 32), UA_STATUSCODE_GOOD);

    /* Second call: prepend2 -> chained IKM written back to slot2.
     * Same ephemeral keys => same shared secret as call 1. */
    ck_assert_int_eq(UA_OpenSSL_ECC_DeriveKeys(
                         EC_curve_nist2nid("P-256"), "SHA256",
                         UA_APPLICATIONTYPE_CLIENT, localKp,
                         &secret2, &seed2, &out),
                     UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);

    /* slot1[i] = prepend1[i] XOR sharedSecret[i]
     * slot2[i] = prepend2[i] XOR sharedSecret[i]
     *   => slot1[i] XOR slot2[i] = prepend1[i] XOR prepend2[i] */
    for(int i = 0; i < 32; i++) {
        UA_Byte xorSlots   = (UA_Byte)(slot1[i] ^ slot2[i]);
        UA_Byte xorPrepends = (UA_Byte)(prepend1[i] ^ prepend2[i]);
        ck_assert_msg(xorSlots == xorPrepends,
                      "IKM chaining invariant violated at byte %d: "
                      "slot1^slot2=0x%02x, prepend1^prepend2=0x%02x",
                      i, xorSlots, xorPrepends);
    }

    /* Also: the slots must have changed (i.e. XOR actually happened).
     * Compare the buffers as a whole — a *per-byte* "changed" assertion
     * would be flaky, since slot[i] == prepend[i] whenever the shared
     * secret byte ss[i] is 0x00 (a ~1/256 event per byte). The XOR only
     * leaves the whole 32-byte buffer unchanged in the astronomically
     * unlikely case that the entire shared secret is zero. */
    ck_assert_msg(memcmp(slot1, prepend1, 32) != 0, "slot1 was not XORed");
    ck_assert_msg(memcmp(slot2, prepend2, 32) != 0, "slot2 was not XORed");

    EVP_PKEY_free(localKp);
    EVP_PKEY_free(remoteKp);
} END_TEST

#endif /* UA_ENABLE_ENCRYPTION_OPENSSL && !LIBRESSL_VERSION_NUMBER */

static Suite *
testSuite_SecureChannel(void) {
    Suite *s = suite_create("SecureChannel");

    TCase *tc_initAndDelete = tcase_create("Initialize and delete Securechannel");
    tcase_add_checked_fixture(tc_initAndDelete, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_initAndDelete, setup_key_sizes, teardown_key_sizes);
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete);
    suite_add_tcase(s, tc_initAndDelete);

    TCase *tc_sendAsymmetricOPNMessage = tcase_create("Test sendAsymmetricOPNMessage function");
    tcase_add_checked_fixture(tc_sendAsymmetricOPNMessage, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_sendAsymmetricOPNMessage, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_sendAsymmetricOPNMessage, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_invalidParameters);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeNone);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_sentDataIsValid);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeSign);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeSignAndEncrypt);
    tcase_add_test(tc_sendAsymmetricOPNMessage,
                   Securechannel_sendAsymmetricOPNMessage_extraPaddingPresentWhenKeyLargerThan2048Bits);
    suite_add_tcase(s, tc_sendAsymmetricOPNMessage);

    TCase *tc_sendSymmetricMessage = tcase_create("Test sendSymmetricMessage function");
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_invalidParameters);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeNone);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeSign);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeSignAndEncrypt);
    suite_add_tcase(s, tc_sendSymmetricMessage);

    TCase *tc_processBuffer = tcase_create("Test chunk assembly");
    tcase_add_checked_fixture(tc_processBuffer, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_processBuffer, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_processBuffer, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_processBuffer, SecureChannel_assemblePartialChunks);
    suite_add_tcase(s, tc_processBuffer);

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) && !defined(LIBRESSL_VERSION_NUMBER)
    TCase *tc_ikmChaining = tcase_create("v1.05.07 IKM chaining on renewal");
    tcase_add_test(tc_ikmChaining, SecureChannel_IKMChaining_prependChainsOnRenewal);
    suite_add_tcase(s, tc_ikmChaining);
#endif

    return s;
}

int
main(void) {
    Suite *s = testSuite_SecureChannel();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
