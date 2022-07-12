/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated.h>
#include <open62541/server_config_default.h>

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_util_internal.h"

#include "check.h"
#include "testing_networklayers.h"
#include "testing_policy.h"

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

UA_SecureChannel testChannel;
UA_ByteString dummyCertificate =
    UA_BYTESTRING_STATIC("DUMMY CERTIFICATE DUMMY CERTIFICATE DUMMY CERTIFICATE");
UA_SecurityPolicy dummyPolicy;
UA_Connection testingConnection;
UA_ByteString sentData;


static funcs_called fCalled;
static key_sizes keySizes;

static void
setup_secureChannel(void) {
    TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled, &keySizes);
    UA_SecureChannel_init(&testChannel, &UA_ConnectionConfig_default);
    UA_SecureChannel_setSecurityPolicy(&testChannel, &dummyPolicy, &dummyCertificate);

    testingConnection =
        createDummyConnection(UA_ConnectionConfig_default.sendBufferSize, &sentData);
    UA_Connection_attachSecureChannel(&testingConnection, &testChannel);
    testChannel.connection = &testingConnection;

    testChannel.state = UA_SECURECHANNELSTATE_OPEN;
}

static void
teardown_secureChannel(void) {
    UA_SecureChannel_close(&testChannel);
    dummyPolicy.clear(&dummyPolicy);
    testingConnection.close(&testingConnection);
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
    UA_SecureChannel_init(&channel, &UA_ConnectionConfig_default);
    retval = UA_SecureChannel_setSecurityPolicy(&channel, &dummyPolicy, &dummyCertificate);

    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode to be good");
    ck_assert_msg(channel.state == UA_SECURECHANNELSTATE_FRESH, "Expected state to be new/fresh");
    ck_assert_msg(fCalled.newContext, "Expected newContext to have been called");
    ck_assert_msg(fCalled.makeCertificateThumbprint,
                  "Expected makeCertificateThumbprint to have been called");
    ck_assert_msg(channel.securityPolicy == &dummyPolicy, "SecurityPolicy not set correctly");

    UA_SecureChannel_close(&channel);
    ck_assert_msg(fCalled.deleteContext, "Expected deleteContext to have been called");

    dummyPolicy.clear(&dummyPolicy);
}END_TEST

static void
createDummyResponse(UA_OpenSecureChannelResponse *response) {
    UA_OpenSecureChannelResponse_init(response);
    memset(response, 0, sizeof(UA_OpenSecureChannelResponse));
}

START_TEST(SecureChannel_sendAsymmetricOPNMessage_withoutConnection) {
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);
    testChannel.securityMode = UA_MESSAGESECURITYMODE_NONE;

    // Remove connection to provoke error
    UA_Connection_detachSecureChannel(testChannel.connection);
    testChannel.connection = NULL;

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);

    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure without a connection");
}END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_invalidParameters) {
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, NULL,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse, NULL);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

}END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeInvalid) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    testChannel.securityMode = UA_MESSAGESECURITYMODE_INVALID;

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_BADSECURITYMODEREJECTED,
                  "Expected SecurityMode rejected error");
}
END_TEST

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeNone) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);
    testChannel.securityMode = UA_MESSAGESECURITYMODE_NONE;

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");
    ck_assert_msg(!fCalled.asym_enc, "Message encryption was called but should not have been");
    ck_assert_msg(!fCalled.asym_sign, "Message signing was called but should not have been");
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION

START_TEST(SecureChannel_sendAsymmetricOPNMessage_SecurityModeSign) {
    // Configure our channel correctly for OPN messages and setup dummy message
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);
    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse,
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
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, 42, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");
    ck_assert_msg(fCalled.asym_enc, "Expected message to have been encrypted but it was not");
    ck_assert_msg(fCalled.asym_sign, "Expected message to have been signed but it was not");
}END_TEST

#endif /* UA_ENABLE_ENCRYPTION */

START_TEST(SecureChannel_sendAsymmetricOPNMessage_sentDataIsValid) {
    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    /* Enable encryption for the SecureChannel */
#ifdef UA_ENABLE_ENCRYPTION
    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
#else
    testChannel.securityMode = UA_MESSAGESECURITYMODE_NONE;
#endif

    UA_UInt32 requestId = UA_UInt32_random();

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, requestId, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");

    size_t offset = 0;
    UA_TcpMessageHeader header;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(&sentData, &offset, &secureChannelId);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &asymSecurityHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(UA_ByteString_equal(&testChannel.securityPolicy->policyUri,
                                      &asymSecurityHeader.securityPolicyUri),
                  "Expected securityPolicyUri to be equal to the one used by the secureChannel");

#ifdef UA_ENABLE_ENCRYPTION
    ck_assert_msg(UA_ByteString_equal(&dummyCertificate, &asymSecurityHeader.senderCertificate),
                  "Expected the certificate to be equal to the one used  by the secureChannel");

    UA_ByteString thumbPrint = {20, testChannel.remoteCertificateThumbprint};
    ck_assert_msg(UA_ByteString_equal(&thumbPrint,
                                      &asymSecurityHeader.receiverCertificateThumbprint),
                  "Expected receiverCertificateThumbprint to be equal to the one set "
                  "in the secureChannel");

    /* Dummy encryption */
    for(size_t i = offset; i < header.messageSize; ++i) {
        sentData.data[i] = (UA_Byte)((sentData.data[i] - 1) % (UA_BYTE_MAX + 1));
    }
#endif

    UA_SequenceHeader sequenceHeader;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &sequenceHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %i but was %i",
                  requestId,
                  sequenceHeader.requestId);

    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(&sentData, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &sentResponse, &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

#ifdef UA_ENABLE_ENCRYPTION
    UA_Byte paddingByte = sentData.data[offset];
    size_t paddingSize = (size_t)paddingByte;

    for(size_t i = 0; i <= paddingSize; ++i) {
        ck_assert_msg(sentData.data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      (int)i, paddingByte, sentData.data[offset + i]);
    }

    ck_assert_msg(sentData.data[offset + paddingSize + 1] == '*', "Expected first byte of signature");
#endif

    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymSecurityHeader);
    UA_SequenceHeader_clear(&sequenceHeader);
    UA_OpenSecureChannelResponse_clear(&sentResponse);
} END_TEST

#ifdef UA_ENABLE_ENCRYPTION

START_TEST(Securechannel_sendAsymmetricOPNMessage_extraPaddingPresentWhenKeyLargerThan2048Bits) {
    keySizes.asym_rmt_enc_key_size = 4096;
    keySizes.asym_rmt_blocksize = 4096;
    keySizes.asym_rmt_ptext_blocksize = 4096;

    UA_OpenSecureChannelResponse dummyResponse;
    createDummyResponse(&dummyResponse);

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_UInt32 requestId = UA_UInt32_random();

    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&testChannel, requestId, &dummyResponse,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to succeed");

    size_t offset = 0;
    UA_TcpMessageHeader header;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(&sentData, &offset, &secureChannelId);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &asymSecurityHeader, &UA_TRANSPORT[UA_TRANSPORT_ASYMMETRICALGORITHMSECURITYHEADER], NULL);
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
        sentData.data[i] = (UA_Byte)((sentData.data[i] - 1) % (UA_BYTE_MAX + 1));
    }

    UA_SequenceHeader sequenceHeader;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &sequenceHeader, &UA_TRANSPORT[UA_TRANSPORT_SEQUENCEHEADER], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %i but was %i",
                  requestId, sequenceHeader.requestId);

    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(&sentData, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    retval = UA_decodeBinaryInternal(&sentData, &offset, &sentResponse, &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

    UA_Byte paddingByte = sentData.data[sentData.length - keySizes.asym_lcl_sig_size - 1];
    size_t paddingSize = (size_t)paddingByte;
    UA_Boolean extraPadding =
        (testChannel.securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
         getRemoteKeyLength(testChannel.channelContext) > 2048);
    UA_Byte extraPaddingByte = 0;
    if(extraPadding) {
        extraPaddingByte = paddingByte;
        paddingByte = sentData.data[sentData.length - keySizes.asym_lcl_sig_size - 2];
        paddingSize = ((size_t)extraPaddingByte << 8u) + paddingByte;
        paddingSize += 1;
    }

    for(size_t i = 0; i < paddingSize; ++i) {
        ck_assert_msg(sentData.data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      (int)i, paddingByte, sentData.data[offset + i]);
    }

    if(extraPadding) {
        ck_assert_msg(sentData.data[offset + paddingSize] == extraPaddingByte,
                      "Expected extra padding byte to be %i but got %i",
                      extraPaddingByte, sentData.data[offset + paddingSize]);
    }
    ck_assert_msg(sentData.data[offset + paddingSize + 1] == '*',
                  "Expected first byte 42 of signature but got %i",
                  sentData.data[offset + paddingSize + 1]);

    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymSecurityHeader);
    UA_SequenceHeader_clear(&sequenceHeader);
    UA_OpenSecureChannelResponse_clear(&sentResponse);
}END_TEST

#endif /* UA_ENABLE_ENCRYPTION */

START_TEST(SecureChannel_sendSymmetricMessage) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    UA_StatusCode retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42, UA_MESSAGETYPE_MSG,
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

    UA_StatusCode retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42, UA_MESSAGETYPE_MSG,
                                                                 &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_msg(!fCalled.sym_sign, "Expected message to not have been signed");
    ck_assert_msg(!fCalled.sym_enc, "Expected message to not have been encrypted");
} END_TEST

#ifdef UA_ENABLE_ENCRYPTION

START_TEST(SecureChannel_sendSymmetricMessage_modeSign) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    testChannel.securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_StatusCode retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42, UA_MESSAGETYPE_MSG,
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

    UA_StatusCode retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42, UA_MESSAGETYPE_MSG,
                                                                 &dummyMessage, &dummyType);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_msg(fCalled.sym_sign, "Expected message to have been signed");
    ck_assert_msg(fCalled.sym_enc, "Expected message to have been encrypted");
} END_TEST

#endif /* UA_ENABLE_ENCRYPTION */

START_TEST(SecureChannel_sendSymmetricMessage_invalidParameters) {
    // initialize dummy message
    UA_ReadRequest dummyMessage;
    UA_ReadRequest_init(&dummyMessage);
    UA_DataType dummyType = UA_TYPES[UA_TYPES_READREQUEST];

    UA_StatusCode retval = UA_SecureChannel_sendSymmetricMessage(NULL, 42, UA_MESSAGETYPE_MSG,
                                                                 &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_HEL, &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_ACK, &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_ERR, &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_OPN, &dummyMessage, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_MSG, NULL, &dummyType);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");

    retval = UA_SecureChannel_sendSymmetricMessage(&testChannel, 42,
                                                   UA_MESSAGETYPE_MSG, &dummyMessage, NULL);
    ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure");
} END_TEST

static UA_StatusCode
process_callback(void *application, UA_SecureChannel *channel,
                 UA_MessageType messageType, UA_UInt32 requestId,
                 UA_ByteString *message) {
    ck_assert_ptr_ne(message, NULL);
    ck_assert_ptr_ne(application, NULL);
    if(message == NULL || application == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    ck_assert_uint_ne(message->length, 0);
    ck_assert_ptr_ne(message->data, NULL);
    int *chunks_processed = (int *)application;
    ++*chunks_processed;
    return UA_STATUSCODE_GOOD;
}

START_TEST(SecureChannel_assemblePartialChunks) {
    int chunks_processed = 0;
    UA_ByteString buffer = UA_BYTESTRING_NULL;

    buffer.data = (UA_Byte *)"HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff";
    buffer.length = 32;

    UA_StatusCode retval = UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 1);

    buffer.length = 16;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 1);

    buffer.data = &buffer.data[16];
    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 2);

    buffer.data = (UA_Byte *)"HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff"
                             "HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff"
                             "HELF \x00\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00"
                             "\x10\x00\x00\x00@\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff";
    buffer.length = 48;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 3);

    buffer.data = &buffer.data[48];
    buffer.length = 32;

    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 4);

    buffer.data = &buffer.data[32];
    buffer.length = 16;
    UA_SecureChannel_processBuffer(&testChannel, &chunks_processed, process_callback, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected success");
    ck_assert_int_eq(chunks_processed, 5);
} END_TEST


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
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_withoutConnection);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_invalidParameters);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeInvalid);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeNone);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_sentDataIsValid);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeSign);
    tcase_add_test(tc_sendAsymmetricOPNMessage, SecureChannel_sendAsymmetricOPNMessage_SecurityModeSignAndEncrypt);
    tcase_add_test(tc_sendAsymmetricOPNMessage,
                   Securechannel_sendAsymmetricOPNMessage_extraPaddingPresentWhenKeyLargerThan2048Bits);
#endif
    suite_add_tcase(s, tc_sendAsymmetricOPNMessage);

    TCase *tc_sendSymmetricMessage = tcase_create("Test sendSymmetricMessage function");
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_sendSymmetricMessage, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_invalidParameters);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeNone);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeSign);
    tcase_add_test(tc_sendSymmetricMessage, SecureChannel_sendSymmetricMessage_modeSignAndEncrypt);
#endif
    suite_add_tcase(s, tc_sendSymmetricMessage);

    TCase *tc_processBuffer = tcase_create("Test chunk assembly");
    tcase_add_checked_fixture(tc_processBuffer, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_processBuffer, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_processBuffer, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_processBuffer, SecureChannel_assemblePartialChunks);
    suite_add_tcase(s, tc_processBuffer);

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
