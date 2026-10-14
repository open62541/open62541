/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/driver/gds_receiver.h>
#include <open62541/driver/file_transfer.h>

#include "ua_server_internal.h"

#include <check.h>
#include "test_helpers.h"
#include "testing_clock.h"
#include "certificates.h"

/* The TrustList Objects of the GDS receiver are served by file transfer
 * drivers (Part 12, 7.8.2). The Methods are called with the callbacks of the
 * nodes, so that the tests can act for different Sessions. */

#define WRITEMODE (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)
#define GROUPID(GROUP, NAME) \
    UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_##GROUP##_TRUSTLIST##NAME

typedef struct {
    UA_UInt32 object, open, close, read, write, getPosition, setPosition,
        openWithMasks, closeAndUpdate, addCertificate, removeCertificate,
        openCount, size, writable, lastModifiedTime, lastUpdateTime,
        maxByteStringLength, activityTimeout;
} TrustListIds;

#define TRUSTLISTIDS(GROUP) {                                            \
    GROUPID(GROUP, ), GROUPID(GROUP, _OPEN), GROUPID(GROUP, _CLOSE),      \
    GROUPID(GROUP, _READ), GROUPID(GROUP, _WRITE),                        \
    GROUPID(GROUP, _GETPOSITION), GROUPID(GROUP, _SETPOSITION),           \
    GROUPID(GROUP, _OPENWITHMASKS), GROUPID(GROUP, _CLOSEANDUPDATE),      \
    GROUPID(GROUP, _ADDCERTIFICATE), GROUPID(GROUP, _REMOVECERTIFICATE),  \
    GROUPID(GROUP, _OPENCOUNT), GROUPID(GROUP, _SIZE),                    \
    GROUPID(GROUP, _WRITABLE), GROUPID(GROUP, _LASTMODIFIEDTIME),         \
    GROUPID(GROUP, _LASTUPDATETIME), GROUPID(GROUP, _MAXBYTESTRINGLENGTH), \
    GROUPID(GROUP, _ACTIVITYTIMEOUT)}

static const TrustListIds appList = TRUSTLISTIDS(DEFAULTAPPLICATIONGROUP);
static const TrustListIds userList = TRUSTLISTIDS(DEFAULTUSERTOKENGROUP);

static UA_Server *server;
static UA_GDSReceiver *receiver;
static const UA_NodeId otherSession = {1, UA_NODEIDTYPE_NUMERIC, {4711}};

static UA_ByteString
intermediateCa(void) {
    UA_ByteString cert = {INTERMEDIATE_CERT_DER_LENGTH, INTERMEDIATE_CERT_DER_DATA};
    return cert;
}

static void
setup(void) {
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    UA_ByteString rootCa = {ROOT_CERT_DER_LENGTH, ROOT_CERT_DER_DATA};
    UA_ByteString crls[2] = {
        {ROOT_EMPTY_CRL_PEM_LENGTH, ROOT_EMPTY_CRL_PEM_DATA},
        {INTERMEDIATE_EMPTY_CRL_PEM_LENGTH, INTERMEDIATE_EMPTY_CRL_PEM_DATA}};
    UA_ByteString cas[2] = {intermediateCa(), rootCa};
    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          cas, 2, cas, 2, crls, 2);
    ck_assert_ptr_nonnull(server);
    UA_Server_getConfig(server)->tcpEnabled = false;
    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &receiver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}

static const UA_NodeId *
admin(void) {
    return &server->adminSession.sessionId;
}

static UA_StatusCode
callAs(const UA_NodeId *session, UA_UInt32 objectId, UA_UInt32 methodId,
       size_t inputSize, UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_NodeId method = UA_NODEID_NUMERIC(0, methodId);
    UA_NodeId object = UA_NODEID_NUMERIC(0, objectId);
    UA_MethodCallback callback = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server, method, &callback),
                      UA_STATUSCODE_GOOD);
    if(!callback)
        return UA_STATUSCODE_BADINTERNALERROR;
    void *methodContext = NULL;
    void *objectContext = NULL;
    UA_Server_getNodeContext(server, method, &methodContext);
    UA_Server_getNodeContext(server, object, &objectContext);
    for(size_t i = 0; i < outputSize; i++)
        UA_Variant_init(&output[i]);
    lockServer(server);
    UA_StatusCode res = callback(server, session, NULL, &method, methodContext,
                                 &object, objectContext, inputSize, input,
                                 outputSize, output);
    unlockServer(server);
    return res;
}

static UA_UInt32
openList(const TrustListIds *tl, const UA_NodeId *session, UA_Byte mode,
         UA_StatusCode expected) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_uint_eq(callAs(session, tl->object, tl->open, 1, &input, 1, &output),
                      expected);
    UA_UInt32 handle = 0;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT32]));
        handle = *(UA_UInt32*)output.data;
    }
    UA_Variant_clear(&output);
    return handle;
}

static UA_UInt32
openWithMasks(const TrustListIds *tl, const UA_NodeId *session, UA_UInt32 masks,
              UA_StatusCode expected) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &masks, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(callAs(session, tl->object, tl->openWithMasks, 1, &input,
                             1, &output), expected);
    UA_UInt32 handle = 0;
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT32]));
        handle = *(UA_UInt32*)output.data;
    }
    UA_Variant_clear(&output);
    return handle;
}

static UA_StatusCode
closeList(const TrustListIds *tl, const UA_NodeId *session, UA_UInt32 handle) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    return callAs(session, tl->object, tl->close, 1, &input, 0, NULL);
}

/* Returns an empty ByteString at the end of the file or on an error */
static UA_StatusCode
readChunk(const TrustListIds *tl, const UA_NodeId *session, UA_UInt32 handle,
          UA_Int32 length, UA_ByteString *out) {
    UA_Variant input[2], output;
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = callAs(session, tl->object, tl->read, 2, input, 1, &output);
    UA_ByteString_init(out);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_BYTESTRING]));
        res = UA_ByteString_copy((UA_ByteString*)output.data, out);
    }
    UA_Variant_clear(&output);
    return res;
}

/* Read up to the empty ByteString at the end of the file */
static UA_ByteString
readAll(const TrustListIds *tl, UA_UInt32 handle) {
    UA_ByteString all = UA_BYTESTRING_NULL;
    size_t length;
    do {
        UA_ByteString chunk;
        ck_assert_uint_eq(readChunk(tl, admin(), handle, 1000, &chunk),
                          UA_STATUSCODE_GOOD);
        length = chunk.length;
        ck_assert_uint_eq(UA_String_append(&all, chunk), UA_STATUSCODE_GOOD);
        UA_ByteString_clear(&chunk);
    } while(length > 0);
    return all;
}

static UA_StatusCode
writeList(const TrustListIds *tl, const UA_NodeId *session, UA_UInt32 handle,
          UA_ByteString data) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &data, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return callAs(session, tl->object, tl->write, 2, input, 0, NULL);
}

static UA_StatusCode
setPosition(const TrustListIds *tl, UA_UInt32 handle, UA_UInt64 position) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &position, &UA_TYPES[UA_TYPES_UINT64]);
    return callAs(admin(), tl->object, tl->setPosition, 2, input, 0, NULL);
}

static UA_UInt64
getPosition(const TrustListIds *tl, UA_UInt32 handle) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(callAs(admin(), tl->object, tl->getPosition, 1, &input, 1, &output),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT64]));
    UA_UInt64 position = *(UA_UInt64*)output.data;
    UA_Variant_clear(&output);
    return position;
}

static UA_StatusCode
closeAndUpdate(const TrustListIds *tl, const UA_NodeId *session, UA_UInt32 handle) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode res = callAs(session, tl->object, tl->closeAndUpdate, 1, &input,
                               1, &output);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_BOOLEAN]));
        ck_assert(*(UA_Boolean*)output.data);
    }
    UA_Variant_clear(&output);
    return res;
}

static UA_StatusCode
applyChanges(const UA_NodeId *session) {
    UA_StatusCode res = callAs(session, UA_NS0ID_SERVERCONFIGURATION,
                               UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL,
                               0, NULL);
    UA_Server_run_iterate(server, false);
    return res;
}

static UA_StatusCode
addCertificate(const TrustListIds *tl) {
    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    UA_Boolean isTrusted = true;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &cert, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[1], &isTrusted, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return callAs(admin(), tl->object, tl->addCertificate, 2, input, 0, NULL);
}

static UA_StatusCode
removeCertificate(const TrustListIds *tl) {
    UA_String thumbprint = UA_STRING("0000000000000000000000000000000000000000");
    UA_Boolean isTrusted = true;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &thumbprint, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[1], &isTrusted, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return callAs(admin(), tl->object, tl->removeCertificate, 2, input, 0, NULL);
}

static UA_UInt16
openCount(const TrustListIds *tl) {
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(0, tl->openCount),
                                          &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    UA_UInt16 count = *(UA_UInt16*)value.data;
    UA_Variant_clear(&value);
    return count;
}

/* An encoded TrustList with only the given trusted certificate */
static UA_ByteString
encodeTrusted(UA_ByteString certificate) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    list.trustedCertificates = &certificate;
    list.trustedCertificatesSize = 1;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&list, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                                      &encoded, NULL), UA_STATUSCODE_GOOD);
    return encoded;
}

static void
decodeList(const UA_ByteString data, UA_TrustListDataType *list) {
    ck_assert_uint_eq(UA_decodeBinary(&data, list, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                                      NULL), UA_STATUSCODE_GOOD);
}

static size_t
trustedCount(UA_CertificateGroup *group) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    lockServer(server);
    ck_assert_uint_eq(group->getTrustList(group, &list), UA_STATUSCODE_GOOD);
    unlockServer(server);
    size_t count = list.trustedCertificatesSize;
    UA_TrustListDataType_clear(&list);
    return count;
}

START_TEST(trustListServedByFileDrivers) {
    ck_assert_uint_eq(openCount(&appList), 0);
    UA_UInt32 h1 = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    UA_UInt32 h2 = openWithMasks(&appList, admin(), UA_TRUSTLISTMASKS_ALL,
                                 UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(h1, h2);
    ck_assert_uint_eq(openCount(&appList), 2);
    ck_assert_uint_eq(openCount(&userList), 0);
    ck_assert_uint_eq(closeList(&appList, admin(), h1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h2), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openCount(&appList), 0);

    /* The Size has no meaning for a TrustList (Part 12, 7.8.2.1) */
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(0, appList.size),
                                          &value), UA_STATUSCODE_BADNOTSUPPORTED);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(0, appList.writable),
                                          &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(*(UA_Boolean*)value.data);
    UA_Variant_clear(&value);

    /* The optional FileType Properties have their standard NodeIds */
    UA_Variant updated;
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, appList.lastModifiedTime), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, appList.lastUpdateTime), &updated),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_DateTime*)value.data, *(UA_DateTime*)updated.data);
    UA_Variant_clear(&value);
    UA_Variant_clear(&updated);
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, userList.maxByteStringLength), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_gt(*(UA_UInt32*)value.data, 0);
    UA_Variant_clear(&value);
} END_TEST

/* Each handle reads the lists of its masks at the time of its Open */
START_TEST(snapshotsAtOpen) {
    UA_UInt32 masks[3] = {UA_TRUSTLISTMASKS_ALL, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES,
                          UA_TRUSTLISTMASKS_ISSUERCERTIFICATES};
    UA_UInt32 handles[3];
    handles[0] = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    for(size_t i = 1; i < 3; i++)
        handles[i] = openWithMasks(&appList, admin(), masks[i], UA_STATUSCODE_GOOD);

    /* A change of the lists does not change the open snapshots */
    UA_CertificateGroup *pki = &UA_Server_getConfig(server)->secureChannelPKI;
    UA_ByteString remaining = intermediateCa();
    UA_TrustListDataType reduced;
    UA_TrustListDataType_init(&reduced);
    reduced.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    reduced.trustedCertificates = &remaining;
    reduced.trustedCertificatesSize = 1;
    lockServer(server);
    ck_assert_uint_eq(pki->setTrustList(pki, &reduced), UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_eq(trustedCount(pki), 1);

    /* Interleaved reads */
    UA_ByteString data[3] = {UA_BYTESTRING_NULL, UA_BYTESTRING_NULL, UA_BYTESTRING_NULL};
    UA_Boolean done[3] = {false, false, false};
    while(!done[0] || !done[1] || !done[2]) {
        for(size_t i = 0; i < 3; i++) {
            if(done[i])
                continue;
            UA_ByteString chunk;
            ck_assert_uint_eq(readChunk(&appList, admin(), handles[i], 100, &chunk),
                              UA_STATUSCODE_GOOD);
            done[i] = (chunk.length == 0);
            ck_assert_uint_eq(UA_String_append(&data[i], chunk), UA_STATUSCODE_GOOD);
            UA_ByteString_clear(&chunk);
        }
    }
    size_t trusted[3] = {2, 2, 0};
    size_t issuers[3] = {2, 0, 2};
    for(size_t i = 0; i < 3; i++) {
        UA_TrustListDataType list;
        decodeList(data[i], &list);
        ck_assert_uint_eq(list.specifiedLists, masks[i]);
        ck_assert_uint_eq(list.trustedCertificatesSize, trusted[i]);
        ck_assert_uint_eq(list.issuerCertificatesSize, issuers[i]);
        if(i > 0) {
            ck_assert_uint_eq(list.trustedCrlsSize, 0);
            ck_assert_uint_eq(list.issuerCrlsSize, 0);
        }
        UA_TrustListDataType_clear(&list);
        UA_ByteString_clear(&data[i]);
        ck_assert_uint_eq(closeList(&appList, admin(), handles[i]), UA_STATUSCODE_GOOD);
    }

    /* A new Open sees the change */
    UA_UInt32 h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    UA_ByteString all = readAll(&appList, h);
    UA_TrustListDataType list;
    decodeList(all, &list);
    ck_assert_uint_eq(list.trustedCertificatesSize, 1);
    UA_TrustListDataType_clear(&list);
    UA_ByteString_clear(&all);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

/* Close discards the written TrustList (Part 12, 7.8.2.1) */
START_TEST(closeDiscardsWrittenData) {
    UA_UInt32 h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&appList, admin(), h, encoded), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&encoded);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_BADNOTHINGTODO);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->secureChannelPKI), 2);
} END_TEST

/* AddCertificate and RemoveCertificate are refused while the TrustList is open */
START_TEST(certificateMethodsWhileOpen) {
    UA_UInt32 h = openWithMasks(&appList, admin(), UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES,
                                UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(addCertificate(&appList), UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(removeCertificate(&appList), UA_STATUSCODE_BADINVALIDSTATE);
    /* Another TrustList is not affected */
    ck_assert_uint_eq(addCertificate(&userList), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(addCertificate(&appList), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->secureChannelPKI), 3);
} END_TEST

/* The written bytes are positional and limited by MaxTrustListSize */
START_TEST(writePositionsAndLimit) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UInt32 h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    config->maxTrustListSize = 10;
    ck_assert_uint_eq(writeList(&appList, admin(), h, UA_BYTESTRING("12345678901")),
                      UA_STATUSCODE_BADREQUESTTOOLARGE);
    ck_assert_uint_eq(openCount(&appList), 1);
    config->maxTrustListSize = 0;
    ck_assert_uint_eq(writeList(&appList, admin(), h, UA_BYTESTRING("garbage")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(getPosition(&appList, h), 7);
    ck_assert_uint_eq(setPosition(&appList, h, 0), UA_STATUSCODE_GOOD);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&appList, admin(), h, encoded), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(getPosition(&appList, h), encoded.length);
    ck_assert_uint_eq(setPosition(&appList, h, (UA_UInt64)1 << 40), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(getPosition(&appList, h), encoded.length);
    UA_ByteString_clear(&encoded);
    ck_assert_uint_eq(closeAndUpdate(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openCount(&appList), 0);
    ck_assert_uint_eq(trustedCount(&config->secureChannelPKI), 2);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustedCount(&config->secureChannelPKI), 1);
} END_TEST

START_TEST(closeAndUpdateFailures) {
    /* Bytes that do not decode stage nothing and keep the handle open */
    UA_UInt32 h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeList(&appList, admin(), h, UA_BYTESTRING("\x01\x02\x03")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeAndUpdate(&appList, admin(), h), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(openCount(&appList), 1);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_BADNOTHINGTODO);

    /* A read handle cannot update */
    h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeAndUpdate(&appList, admin(), h), UA_STATUSCODE_BADINVALIDSTATE);

    /* Handles belong to their TrustList and Session */
    UA_ByteString chunk;
    ck_assert_uint_eq(readChunk(&userList, admin(), h, 10, &chunk),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(readChunk(&appList, &otherSession, h, 10, &chunk),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(closeAndUpdate(&userList, admin(), h),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(closeList(&userList, admin(), h), UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeAndUpdate(&appList, admin(), h),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(userTokenGroupTrustList) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t applicationTrusted = trustedCount(&config->secureChannelPKI);
    UA_UInt32 h = openList(&userList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&userList, admin(), h, encoded), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&encoded);
    ck_assert_uint_eq(closeAndUpdate(&userList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustedCount(&config->sessionPKI), 1);
    ck_assert_uint_eq(trustedCount(&config->secureChannelPKI), applicationTrusted);
} END_TEST

/* Only the Session of the transaction can open a TrustList (Part 12, 7.8.2.2).
 * While a handle is open, the file locking of Part 20 decides first. */
START_TEST(transactionOfAnotherSession) {
    UA_UInt32 h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&appList, admin(), h, encoded), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&encoded);
    openList(&appList, &otherSession, UA_OPENFILEMODE_READ, UA_STATUSCODE_BADNOTREADABLE);
    openList(&userList, &otherSession, UA_OPENFILEMODE_READ,
             UA_STATUSCODE_BADTRANSACTIONPENDING);
    openList(&userList, &otherSession, WRITEMODE, UA_STATUSCODE_BADTRANSACTIONPENDING);
    ck_assert_uint_eq(closeAndUpdate(&appList, admin(), h), UA_STATUSCODE_GOOD);

    openList(&appList, &otherSession, UA_OPENFILEMODE_READ,
             UA_STATUSCODE_BADTRANSACTIONPENDING);
    openWithMasks(&appList, &otherSession, UA_TRUSTLISTMASKS_ALL,
                  UA_STATUSCODE_BADTRANSACTIONPENDING);
    openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_BADTRANSACTIONPENDING);

    /* The Session of the transaction continues it, Close keeps the staged list */
    h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(&otherSession), UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->secureChannelPKI), 1);
} END_TEST

/* A transaction without changes ends with its last TrustList open for
 * writing */
START_TEST(transactionKeptWhileWriting) {
    UA_UInt32 h1 = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    UA_UInt32 h2 = openList(&userList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h1), UA_STATUSCODE_GOOD);
    openList(&appList, &otherSession, WRITEMODE, UA_STATUSCODE_BADTRANSACTIONPENDING);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&userList, admin(), h2, encoded), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&encoded);
    /* CloseAndUpdate only applies to the transaction of the Session */
    ck_assert_uint_eq(closeAndUpdate(&userList, &otherSession, h2),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(closeAndUpdate(&userList, admin(), h2), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->sessionPKI), 1);

    /* Without changes, closing the last write ends the transaction */
    h1 = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    h2 = openList(&userList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&userList, admin(), h2), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_BADNOTHINGTODO);
} END_TEST

/* A MaxTrustListSize above the default chunk size allows a whole TrustList in
 * one Write */
START_TEST(writeChunkFollowsMaxTrustListSize) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->maxTrustListSize = 2u << 20;
    lockServer(server);
    receiver->drv.stop(&receiver->drv);
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, appList.maxByteStringLength), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 2u << 20);
    UA_Variant_clear(&value);
    UA_UInt32 h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    UA_ByteString chunk;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&chunk, 3u << 19), UA_STATUSCODE_GOOD);
    memset(chunk.data, 0, chunk.length);
    ck_assert_uint_eq(writeList(&appList, admin(), h, chunk), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&chunk);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

/* Only Read and Write + EraseExisting open a TrustList (Part 12, 7.8.2.2).
 * The driver rejects invalid bit combinations before (Part 20, 4.2.2). */
START_TEST(openModes) {
    UA_Byte unsupported[4] = {UA_OPENFILEMODE_WRITE,
                              UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                              UA_OPENFILEMODE_READ | WRITEMODE,
                              UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_APPEND};
    for(size_t i = 0; i < 4; i++)
        openList(&appList, admin(), unsupported[i], UA_STATUSCODE_BADNOTSUPPORTED);
    UA_Byte invalid[4] = {0, UA_OPENFILEMODE_ERASEEXISTING,
                          UA_OPENFILEMODE_READ | UA_OPENFILEMODE_ERASEEXISTING, 0x10};
    for(size_t i = 0; i < 4; i++)
        openList(&appList, admin(), invalid[i], UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(openCount(&appList), 0);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_BADNOTHINGTODO);

    UA_UInt32 h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

static void
closeSession(const UA_NodeId *session) {
    UA_KeyValueMap payload = UA_KEYVALUEMAP_NULL;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&payload, UA_QUALIFIEDNAME(0, "session-id"),
                          (void*)(uintptr_t)session, &UA_TYPES[UA_TYPES_NODEID]),
                      UA_STATUSCODE_GOOD);
    lockServer(server);
    notifyApplication(server, UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED, payload);
    unlockServer(server);
    UA_KeyValueMap_clear(&payload);
}

/* A closed or abandoned Session discards its open TrustLists and its
 * transaction (Part 12, 7.10.9) */
START_TEST(sessionCloseDiscardsTransaction) {
    UA_UInt32 h = openList(&appList, &otherSession, WRITEMODE, UA_STATUSCODE_GOOD);
    UA_ByteString encoded = encodeTrusted(intermediateCa());
    ck_assert_uint_eq(writeList(&appList, &otherSession, h, encoded), UA_STATUSCODE_GOOD);
    closeSession(&otherSession);
    ck_assert_uint_eq(openCount(&appList), 0);
    ck_assert_uint_eq(applyChanges(&otherSession), UA_STATUSCODE_BADNOTHINGTODO);

    /* Also with a staged TrustList */
    h = openList(&appList, &otherSession, WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeList(&appList, &otherSession, h, encoded), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeAndUpdate(&appList, &otherSession, h), UA_STATUSCODE_GOOD);
    closeSession(admin());
    openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_BADTRANSACTIONPENDING);
    closeSession(&otherSession);
    h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(applyChanges(&otherSession), UA_STATUSCODE_BADNOTHINGTODO);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->secureChannelPKI), 2);

    /* A queued ApplyChanges completes */
    h = openList(&appList, &otherSession, WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeList(&appList, &otherSession, h, encoded), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeAndUpdate(&appList, &otherSession, h), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&encoded);
    ck_assert_uint_eq(callAs(&otherSession, UA_NS0ID_SERVERCONFIGURATION,
                             UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL, 0, NULL),
                      UA_STATUSCODE_GOOD);
    closeSession(&otherSession);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(trustedCount(&UA_Server_getConfig(server)->secureChannelPKI), 1);
    h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

static size_t
fileTransferDrivers(void) {
    size_t count = 0;
    UA_String name = UA_STRING("file-transfer");
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(UA_String_equal(&drv->name, &name))
            count++;
    }
    return count;
}

static void
advanceClock(UA_UInt32 ms) {
    UA_fakeSleep(ms);
    UA_Server_run_iterate(server, false);
}

static UA_StatusCode
readActivityTimeout(const TrustListIds *tl, UA_Double *timeout) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(0, tl->activityTimeout), &value);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DURATION]));
        *timeout = *(UA_Double*)value.data;
    }
    UA_Variant_clear(&value);
    return res;
}

static UA_StatusCode
restartReceiver(const void *timeout, const UA_DataType *type) {
    lockServer(server);
    receiver->drv.stop(&receiver->drv);
    unlockServer(server);
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "activity-timeout");
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&receiver->drv.params, key, timeout, type),
                      UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_StatusCode res = receiver->drv.start(&receiver->drv);
    unlockServer(server);
    return res;
}

/* An idle TrustList closes after the ActivityTimeout and discards its changes
 * (Part 12, 7.8.2.1) */
START_TEST(activityTimeoutClosesTrustList) {
    UA_Double timeout = 0;
    ck_assert_uint_eq(readActivityTimeout(&appList, &timeout), UA_STATUSCODE_GOOD);
    ck_assert(timeout == 60000.0);
    UA_UInt32 h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    advanceClock(50000);
    UA_ByteString chunk;
    ck_assert_uint_eq(readChunk(&appList, admin(), h, 10, &chunk), UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&chunk);
    advanceClock(50000);
    ck_assert_uint_eq(openCount(&appList), 1);
    advanceClock(120001);
    ck_assert_uint_eq(openCount(&appList), 0);

    /* An idle write ends its transaction */
    h = openList(&appList, admin(), WRITEMODE, UA_STATUSCODE_GOOD);
    advanceClock(120001);
    ck_assert_uint_eq(openCount(&appList), 0);
    ck_assert_uint_eq(applyChanges(admin()), UA_STATUSCODE_BADNOTHINGTODO);
    h = openList(&appList, &otherSession, UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, &otherSession, h), UA_STATUSCODE_GOOD);

    /* The parameter of the receiver */
    UA_Double shortTimeout = 100;
    ck_assert_uint_eq(restartReceiver(&shortTimeout, &UA_TYPES[UA_TYPES_DOUBLE]),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readActivityTimeout(&userList, &timeout), UA_STATUSCODE_GOOD);
    ck_assert(timeout == 100.0);
    h = openList(&userList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    advanceClock(201);
    ck_assert_uint_eq(openCount(&userList), 0);

    /* 0 disables it and removes the Property */
    UA_Double disabled = 0;
    ck_assert_uint_eq(restartReceiver(&disabled, &UA_TYPES[UA_TYPES_DOUBLE]),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readActivityTimeout(&appList, &timeout),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    advanceClock(10 * 60 * 1000);
    ck_assert_uint_eq(openCount(&appList), 1);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);

    /* Invalid values and another type fail the start */
    UA_Double negative = -1;
    ck_assert_uint_eq(restartReceiver(&negative, &UA_TYPES[UA_TYPES_DOUBLE]),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(fileTransferDrivers(), 0);
    UA_UInt32 wrongType = 100;
    ck_assert_uint_eq(restartReceiver(&wrongType, &UA_TYPES[UA_TYPES_UINT32]),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(restartReceiver(&shortTimeout, &UA_TYPES[UA_TYPES_DOUBLE]),
                      UA_STATUSCODE_GOOD);
} END_TEST


/* The receiver registers the file transfer drivers while it is started */
START_TEST(fileDriversFollowReceiver) {
    ck_assert_uint_eq(fileTransferDrivers(), 2);
    UA_UInt32 h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    lockServer(server);
    receiver->drv.stop(&receiver->drv);
    unlockServer(server);
    ck_assert_uint_eq(fileTransferDrivers(), 0);
    UA_MethodCallback callback = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server,
                          UA_NODEID_NUMERIC(0, appList.open), &callback),
                      UA_STATUSCODE_GOOD);
    ck_assert(callback == NULL);
    void *context = &callback;
    ck_assert_uint_eq(UA_Server_getNodeContext(server, UA_NODEID_NUMERIC(0, appList.object),
                                               &context), UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(context);

    lockServer(server);
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_eq(fileTransferDrivers(), 2);
    ck_assert_uint_eq(openCount(&appList), 0);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_BADINVALIDARGUMENT);
    h = openList(&appList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&appList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

/* A receiver added to a running server serves the TrustLists right away */
START_TEST(receiverAddedAtRuntime) {
    lockServer(server);
    receiver->drv.stop(&receiver->drv);
    unlockServer(server);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &receiver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.free(&receiver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fileTransferDrivers(), 0);

    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &receiver->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(fileTransferDrivers(), 2);
    UA_UInt32 h = openList(&userList, admin(), UA_OPENFILEMODE_READ, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeList(&userList, admin(), h), UA_STATUSCODE_GOOD);
} END_TEST

static UA_StatusCode
stubOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
         const UA_FileTransferOpenContext *context, UA_UInt32 *handle) {
    *handle = 1;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubRead(UA_FileTransferFileBackend *b, UA_UInt32 handle, UA_Int32 length,
         UA_ByteString *out) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                UA_UInt64 *position) {
    *position = 0;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
                UA_UInt64 position) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
            UA_FileTransferFileInfo *info) {
    info->accessRights = UA_FILEACCESS_READ;
    return UA_STATUSCODE_GOOD;
}

/* A TrustList that another file driver serves fails the start completely */
START_TEST(receiverStartRollsBack) {
    lockServer(server);
    receiver->drv.stop(&receiver->drv);
    unlockServer(server);
    UA_FileTransferFileBackend backend;
    memset(&backend, 0, sizeof(backend));
    backend.open = stubOpen;
    backend.close = stubClose;
    backend.read = stubRead;
    backend.getPosition = stubGetPosition;
    backend.setPosition = stubSetPosition;
    backend.getInfo = stubGetInfo;
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = UA_NODEID_NUMERIC(0, userList.object);
    UA_Driver *foreign = NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server, &backend, UA_STRING_NULL,
                          &description, NULL, &foreign), UA_STATUSCODE_GOOD);
    UA_Boolean readOnly = true;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&foreign->params,
                          UA_QUALIFIEDNAME(0, "read-only"), &readOnly,
                          &UA_TYPES[UA_TYPES_BOOLEAN]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, foreign), UA_STATUSCODE_GOOD);

    lockServer(server);
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_BADNODEIDEXISTS);
    unlockServer(server);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(fileTransferDrivers(), 1);
    UA_MethodCallback callback = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server,
                          UA_NODEID_NUMERIC(0, appList.openWithMasks), &callback),
                      UA_STATUSCODE_GOOD);
    ck_assert(callback == NULL);

    lockServer(server);
    foreign->stop(foreign);
    unlockServer(server);
    ck_assert_uint_eq(UA_Server_removeDriver(server, foreign), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(foreign->free(foreign), UA_STATUSCODE_GOOD);
    lockServer(server);
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_eq(fileTransferDrivers(), 2);
} END_TEST

int main(void) {
    Suite *s = suite_create("GDS TrustList");
    TCase *tc = tcase_create("TrustList File");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, trustListServedByFileDrivers);
    tcase_add_test(tc, snapshotsAtOpen);
    tcase_add_test(tc, closeDiscardsWrittenData);
    tcase_add_test(tc, certificateMethodsWhileOpen);
    tcase_add_test(tc, writePositionsAndLimit);
    tcase_add_test(tc, closeAndUpdateFailures);
    tcase_add_test(tc, userTokenGroupTrustList);
    tcase_add_test(tc, transactionOfAnotherSession);
    tcase_add_test(tc, transactionKeptWhileWriting);
    tcase_add_test(tc, writeChunkFollowsMaxTrustListSize);
    tcase_add_test(tc, openModes);
    tcase_add_test(tc, sessionCloseDiscardsTransaction);
    tcase_add_test(tc, activityTimeoutClosesTrustList);
    tcase_add_test(tc, fileDriversFollowReceiver);
    tcase_add_test(tc, receiverAddedAtRuntime);
    tcase_add_test(tc, receiverStartRollsBack);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
