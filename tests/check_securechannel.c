/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_encoding_binary.h>

#include "ua_securechannel.h"
#include <ua_types_encoding_binary.h>

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
    UA_SecureChannel_init(&testChannel);
    UA_SecureChannel_setSecurityPolicy(&testChannel, &dummyPolicy, &dummyCertificate);

    testingConnection = createDummyConnection(65535, &sentData);
    UA_Connection_attachSecureChannel(&testingConnection, &testChannel);
    testChannel.connection = &testingConnection;
}

static void
teardown_secureChannel(void) {
    UA_SecureChannel_close(&testChannel);
    UA_SecureChannel_deleteMembers(&testChannel);
    dummyPolicy.deleteMembers(&dummyPolicy);
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
    UA_SecureChannel_init(&channel);
    retval = UA_SecureChannel_setSecurityPolicy(&channel, &dummyPolicy, &dummyCertificate);

    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode to be good");
    ck_assert_msg(channel.state == UA_SECURECHANNELSTATE_FRESH, "Expected state to be fresh");
    ck_assert_msg(fCalled.newContext, "Expected newContext to have been called");
    ck_assert_msg(fCalled.makeCertificateThumbprint,
                  "Expected makeCertificateThumbprint to have been called");
    ck_assert_msg(channel.securityPolicy == &dummyPolicy, "SecurityPolicy not set correctly");

    UA_SecureChannel_close(&channel);
    UA_SecureChannel_deleteMembers(&channel);
    ck_assert_msg(fCalled.deleteContext, "Expected deleteContext to have been called");

    dummyPolicy.deleteMembers(&dummyPolicy);
}END_TEST

START_TEST(SecureChannel_generateNewKeys) {
    UA_StatusCode retval = UA_SecureChannel_generateNewKeys(&testChannel);

    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected Statuscode to be good");
    ck_assert_msg(fCalled.generateKey, "Expected generateKey to have been called");
    ck_assert_msg(fCalled.setLocalSymEncryptingKey,
                  "Expected setLocalSymEncryptingKey to have been called");
    ck_assert_msg(fCalled.setLocalSymSigningKey,
                  "Expected setLocalSymSigningKey to have been called");
    ck_assert_msg(fCalled.setLocalSymIv, "Expected setLocalSymIv to have been called");
    ck_assert_msg(fCalled.setRemoteSymEncryptingKey,
                  "Expected setRemoteSymEncryptingKey to have been called");
    ck_assert_msg(fCalled.setRemoteSymSigningKey,
                  "Expected setRemoteSymSigningKey to have been called");
    ck_assert_msg(fCalled.setRemoteSymIv, "Expected setRemoteSymIv to have been called");
}END_TEST

START_TEST(SecureChannel_revolveTokens) {
    // Fake that no token was issued by setting 0
    testChannel.nextSecurityToken.tokenId = 0;
    UA_StatusCode retval = UA_SecureChannel_revolveTokens(&testChannel);
    ck_assert_msg(retval == UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN,
                  "Expected failure because tokenId 0 signifies that no token was issued");

    // Fake an issued token by setting an id
    testChannel.nextSecurityToken.tokenId = 10;
    retval = UA_SecureChannel_revolveTokens(&testChannel);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to return GOOD");
    ck_assert_msg(fCalled.generateKey,
                  "Expected generateKey to be called because new keys need to be generated,"
                  "when switching to the next token.");

    UA_ChannelSecurityToken testToken;
    UA_ChannelSecurityToken_init(&testToken);

    ck_assert_msg(memcmp(&testChannel.nextSecurityToken, &testToken,
                         sizeof(UA_ChannelSecurityToken)) == 0,
                  "Expected the next securityToken to be freshly initialized");
    ck_assert_msg(testChannel.securityToken.tokenId == 10, "Expected token to have been copied");
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
    UA_SecureConversationMessageHeader header;
    UA_SecureConversationMessageHeader_decodeBinary(&sentData, &offset, &header);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&sentData, &offset, &asymSecurityHeader);

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
    for(size_t i = offset; i < header.messageHeader.messageSize; ++i) {
        sentData.data[i] = (UA_Byte)((sentData.data[i] - 1) % (UA_BYTE_MAX + 1));
    }
#endif

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_decodeBinary(&sentData, &offset, &sequenceHeader);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %i but was %i",
                  requestId,
                  sequenceHeader.requestId);

    UA_NodeId original =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(&sentData, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&original, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    UA_OpenSecureChannelResponse_decodeBinary(&sentData, &offset, &sentResponse);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

#ifdef UA_ENABLE_ENCRYPTION
    UA_Byte paddingByte = sentData.data[offset];
    size_t paddingSize = (size_t)paddingByte;

    for(size_t i = 0; i <= paddingSize; ++i) {
        ck_assert_msg(sentData.data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      i, paddingByte, sentData.data[offset + i]);
    }

    ck_assert_msg(sentData.data[offset + paddingSize + 1] == '*', "Expected first byte of signature");
#endif

    UA_SecureConversationMessageHeader_deleteMembers(&header);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymSecurityHeader);
    UA_SequenceHeader_deleteMembers(&sequenceHeader);
    UA_OpenSecureChannelResponse_deleteMembers(&sentResponse);
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
    UA_SecureConversationMessageHeader header;
    UA_SecureConversationMessageHeader_decodeBinary(&sentData, &offset, &header);

    UA_AsymmetricAlgorithmSecurityHeader asymSecurityHeader;
    UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&sentData, &offset, &asymSecurityHeader);
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

    for(size_t i = offset; i < header.messageHeader.messageSize; ++i) {
        sentData.data[i] = (UA_Byte)((sentData.data[i] - 1) % (UA_BYTE_MAX + 1));
    }

    UA_SequenceHeader sequenceHeader;
    UA_SequenceHeader_decodeBinary(&sentData, &offset, &sequenceHeader);
    ck_assert_msg(sequenceHeader.requestId == requestId, "Expected requestId to be %i but was %i",
                  requestId, sequenceHeader.requestId);

    UA_NodeId original =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    UA_NodeId requestTypeId;
    UA_NodeId_decodeBinary(&sentData, &offset, &requestTypeId);
    ck_assert_msg(UA_NodeId_equal(&original, &requestTypeId), "Expected nodeIds to be equal");

    UA_OpenSecureChannelResponse sentResponse;
    UA_OpenSecureChannelResponse_decodeBinary(&sentData, &offset, &sentResponse);

    ck_assert_msg(memcmp(&sentResponse, &dummyResponse, sizeof(UA_OpenSecureChannelResponse)) == 0,
                  "Expected the sent response to be equal to the one supplied to the send function");

    UA_Byte paddingByte = sentData.data[offset];
    UA_Byte extraPaddingByte = sentData.data[sentData.length - keySizes.asym_lcl_sig_size - 1];
    size_t paddingSize = (size_t)paddingByte;
    paddingSize |= extraPaddingByte << 8;

    for(size_t i = 0; i <= paddingSize; ++i) {
        ck_assert_msg(sentData.data[offset + i] == paddingByte,
                      "Expected padding byte %i to be %i but got value %i",
                      i,
                      paddingByte,
                      sentData.data[offset + i]);
    }

    ck_assert_msg(sentData.data[offset + paddingSize + 1] == extraPaddingByte,
                  "Expected extra padding byte to be %i but got %i",
                  extraPaddingByte, sentData.data[offset + paddingSize + 1]);
    ck_assert_msg(sentData.data[offset + paddingSize + 2] == '*',
                  "Expected first byte 42 of signature but got %i",
                  sentData.data[offset + paddingSize + 2]);

    UA_SecureConversationMessageHeader_deleteMembers(&header);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymSecurityHeader);
    UA_SequenceHeader_deleteMembers(&sequenceHeader);
    UA_OpenSecureChannelResponse_deleteMembers(&sentResponse);
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

static Suite *
testSuite_SecureChannel(void) {
    Suite *s = suite_create("SecureChannel");

    TCase *tc_initAndDelete = tcase_create("Initialize and delete Securechannel");
    tcase_add_checked_fixture(tc_initAndDelete, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_initAndDelete, setup_key_sizes, teardown_key_sizes);
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete);
    suite_add_tcase(s, tc_initAndDelete);

    TCase *tc_generateNewKeys = tcase_create("Test generateNewKeys function");
    tcase_add_checked_fixture(tc_generateNewKeys, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_generateNewKeys, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_generateNewKeys, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_generateNewKeys, SecureChannel_generateNewKeys);
    suite_add_tcase(s, tc_generateNewKeys);

    TCase *tc_revolveTokens = tcase_create("Test revolveTokens function");
    tcase_add_checked_fixture(tc_revolveTokens, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_revolveTokens, setup_key_sizes, teardown_key_sizes);
    tcase_add_checked_fixture(tc_revolveTokens, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_revolveTokens, SecureChannel_revolveTokens);
    suite_add_tcase(s, tc_revolveTokens);

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
