/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Lifecycle tests for the GDS Pull Requester driver. */

#include <open62541/driver/gds_pull.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include "thread_wrapper.h"

#define REQUESTER_PORT 14840
#define GDS_PORT 14841
#define GDS_ENDPOINT "opc.tcp://localhost:14841"
#define UNUSED_ENDPOINT "opc.tcp://localhost:14842"

#define REQUESTER_URI "urn:open62541.test.gds-pull-requester"
#define GDS_URI "urn:open62541.test.certificate-manager"

/* Well-known NodeIds of the GDS namespace (Opc.Ua.Gds.NodeIds.csv) */
#define GDS_NS_URI "http://opcfoundation.org/UA/GDS/"
#define GDSID_DIRECTORY 141
#define GDSID_DIRECTORY_FINDAPPLICATIONS 143
#define GDSID_DIRECTORY_REGISTERAPPLICATION 146
#define GDSID_DIRECTORY_STARTNEWKEYPAIRREQUEST 154
#define GDSID_DIRECTORY_STARTSIGNINGREQUEST 157
#define GDSID_DIRECTORY_FINISHREQUEST 163
#define GDSID_DIRECTORY_GETTRUSTLIST 204
#define GDSID_DIRECTORY_GETCERTIFICATESTATUS 225
#define GDSID_DIRECTORY_GETCERTIFICATEGROUPS 508
#define GDSID_DIRECTORY_CERTIFICATEGROUPS 614
#define GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP 615
#define GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST 616

/* Identifiers handed out by the mock CertificateManager */
#define GDSID_APPLICATION 1000
#define GDSID_FIRST_REQUEST 2000

/* Interval of the driver cycles and the real-time budget for waiting on a
 * notification */
#define CYCLE_INTERVAL_MS 500
#define WAIT_TIMEOUT_MS 60000

#define MAX_NOTIFICATIONS 128
#define MAX_REQUESTS 8

/* Certificates are created once for the whole suite */
static UA_ByteString requesterCert, requesterKey;
static UA_ByteString gdsCert, gdsKey;
static UA_ByteString foreignCert, foreignKey;

static const UA_NodeId defaultApplicationGroup = {
    0, UA_NODEIDTYPE_NUMERIC,
    {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP}};
static const UA_NodeId rsaSha256CertificateType = {
    0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE}};

/******************************/
/* Mock CertificateManager    */
/******************************/

typedef enum {
    FINISH_APPROVE, /* FinishRequest returns the issued certificate */
    FINISH_PENDING, /* FinishRequest returns Bad_NothingToDo */
    FINISH_REJECT   /* FinishRequest returns Bad_RequestNotAllowed */
} FinishMode;

typedef struct {
    UA_Server *server;
    UA_UInt16 ns;
    UA_NodeId applicationId;
    UA_NodeId groupId;
    UA_NodeId trustListId;
    UA_NodeId trustListSizeId;
    UA_NodeId trustListLastUpdateTimeId;

    /* Configurable behaviour. UpdateRequired is reported until a certificate
     * has been issued. */
    UA_Boolean updateRequired;
    FinishMode finishMode;
    UA_ByteString issuedCertificate; /* not owned */

    /* Served TrustList: a raw UA Binary encoded TrustListDataType (7.8.2.1) */
    UA_TrustListDataType trustList;
    UA_ByteString trustListFile;

    /* A single open file handle is sufficient */
    UA_UInt32 nextHandle;
    UA_UInt32 openHandle;
    size_t position;

    /* Outstanding signing requests */
    UA_UInt32 nextRequest;
    size_t requestsSize;
    UA_NodeId requestIds[MAX_REQUESTS];
    UA_NodeId lastRequestId;
    UA_NodeId lastCertificateType;
    UA_ByteString lastCsr;

    /* Call counters */
    size_t findApplications, registerApplication, getCertificateGroups,
        getCertificateStatus, startSigningRequest, startNewKeyPairRequest,
        finishRequest, getTrustList, open, read, close;
} MockCertificateManager;

static MockCertificateManager cm;

/* All access to the mock state from the Method callbacks (CertificateManager
 * thread) and from the tests is serialized by cmMutex. */
static MUTEX_HANDLE cmMutex;
static THREAD_HANDLE cmThread;
static volatile UA_Boolean cmRunning;

static void
lockCm(void) {
    (void)MUTEX_LOCK(cmMutex);
}

static void
unlockCm(void) {
    (void)MUTEX_UNLOCK(cmMutex);
}

/* Shallow copy of the mock state for assertions */
static MockCertificateManager
snapshot(void) {
    lockCm();
    MockCertificateManager copy = cm;
    unlockCm();
    return copy;
}

static void
setFinishMode(FinishMode mode) {
    lockCm();
    cm.finishMode = mode;
    unlockCm();
}

static void
setUpdateRequired(UA_Boolean updateRequired) {
    lockCm();
    cm.updateRequired = updateRequired;
    unlockCm();
}

static void
setIssuedCertificate(UA_ByteString certificate) {
    lockCm();
    cm.issuedCertificate = certificate;
    unlockCm();
}

/* Method callbacks run in the CertificateManager thread. They must not use
 * the check assertions. */
#define MOCK_METHOD(name)                                                    \
    static UA_StatusCode name##Locked(const UA_Variant *input,               \
                                      UA_Variant *output);                   \
    static UA_StatusCode name(                                               \
        UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,      \
        const UA_NodeId *methodId, void *methodContext,                      \
        const UA_NodeId *objectId, void *objectContext, size_t inputSize,    \
        const UA_Variant *input, size_t outputSize, UA_Variant *output) {    \
        lockCm();                                                            \
        UA_StatusCode res = name##Locked(input, output);                     \
        unlockCm();                                                          \
        return res;                                                          \
    }                                                                        \
    static UA_StatusCode name##Locked(const UA_Variant *input,               \
                                      UA_Variant *output)

static UA_Boolean
isScalar(const UA_Variant *v, const UA_DataType *type) {
    return UA_Variant_hasScalarType(v, type) && v->data != NULL;
}

static UA_StatusCode
checkApplicationId(const UA_Variant *v) {
    if(!isScalar(v, &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!UA_NodeId_equal((const UA_NodeId*)v->data, &cm.applicationId))
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_STATUSCODE_GOOD;
}

/* A null CertificateGroupId selects the DefaultApplicationGroup */
static UA_StatusCode
checkCertificateGroupId(const UA_Variant *v) {
    if(!isScalar(v, &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_NodeId *id = (const UA_NodeId*)v->data;
    if(UA_NodeId_isNull(id) || UA_NodeId_equal(id, &cm.groupId))
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

/* Registration (6.5) is not part of the Pull Management Workflow */
MOCK_METHOD(mockFindApplications) {
    cm.findApplications++;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

MOCK_METHOD(mockRegisterApplication) {
    cm.registerApplication++;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

/* 7.9.7: GetCertificateGroups(ApplicationId) -> CertificateGroupIds[] */
MOCK_METHOD(mockGetCertificateGroups) {
    cm.getCertificateGroups++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setArrayCopy(&output[0], &cm.groupId, 1,
                                   &UA_TYPES[UA_TYPES_NODEID]);
}

/* 7.9.10: GetCertificateStatus(ApplicationId, CertificateGroupId,
 *                               CertificateTypeId) -> UpdateRequired */
MOCK_METHOD(mockGetCertificateStatus) {
    cm.getCertificateStatus++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &cm.updateRequired,
                                    &UA_TYPES[UA_TYPES_BOOLEAN]);
}

/* Must be called with cmMutex held */
static UA_NodeId
addRequest(void) {
    if(cm.requestsSize >= MAX_REQUESTS)
        return UA_NODEID_NULL;
    UA_NodeId id = UA_NODEID_NUMERIC(cm.ns, GDSID_FIRST_REQUEST + cm.nextRequest++);
    cm.requestIds[cm.requestsSize++] = id;
    cm.lastRequestId = id;
    return id;
}

/* Must be called with cmMutex held */
static UA_Boolean
removeRequest(const UA_NodeId *id) {
    for(size_t i = 0; i < cm.requestsSize; i++) {
        if(!UA_NodeId_equal(&cm.requestIds[i], id))
            continue;
        cm.requestIds[i] = cm.requestIds[--cm.requestsSize];
        return true;
    }
    return false;
}

/* 7.9.3: StartSigningRequest(ApplicationId, CertificateGroupId,
 *                             CertificateTypeId, CertificateRequest)
 *        -> RequestId */
MOCK_METHOD(mockStartSigningRequest) {
    cm.startSigningRequest++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[2], &UA_TYPES[UA_TYPES_NODEID]) ||
       !isScalar(&input[3], &UA_TYPES[UA_TYPES_BYTESTRING]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_ByteString *csr = (const UA_ByteString*)input[3].data;
    if(csr->length == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString_clear(&cm.lastCsr);
    res = UA_ByteString_copy(csr, &cm.lastCsr);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_NodeId requestId = addRequest();
    if(UA_NodeId_isNull(&requestId))
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    cm.lastCertificateType = *(const UA_NodeId*)input[2].data;
    return UA_Variant_setScalarCopy(&output[0], &requestId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

MOCK_METHOD(mockStartNewKeyPairRequest) {
    cm.startNewKeyPairRequest++;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

/* 7.9.5: FinishRequest(ApplicationId, RequestId)
 *        -> Certificate, PrivateKey, IssuerCertificates[] */
MOCK_METHOD(mockFinishRequest) {
    cm.finishRequest++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[1], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_NodeId *requestId = (const UA_NodeId*)input[1].data;

    UA_Boolean known = false;
    for(size_t i = 0; i < cm.requestsSize; i++)
        known |= UA_NodeId_equal(&cm.requestIds[i], requestId);
    if(!known)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    switch(cm.finishMode) {
    case FINISH_PENDING:
        return UA_STATUSCODE_BADNOTHINGTODO;
    case FINISH_REJECT:
        removeRequest(requestId);
        return UA_STATUSCODE_BADREQUESTNOTALLOWED;
    case FINISH_APPROVE:
    default:
        break;
    }

    removeRequest(requestId);
    cm.updateRequired = false; /* the application is up to date from now on */
    UA_ByteString noPrivateKey = UA_BYTESTRING_NULL;
    res = UA_Variant_setScalarCopy(&output[0], &cm.issuedCertificate,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_Variant_setScalarCopy(&output[1], &noPrivateKey,
                                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_Variant_setArrayCopy(&output[2], NULL, 0,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    return res;
}

/* 7.9.9: GetTrustList(ApplicationId, CertificateGroupId) -> TrustListId */
MOCK_METHOD(mockGetTrustList) {
    cm.getTrustList++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Variant_setScalarCopy(&output[0], &cm.trustListId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

/* FileType (OPC 10000-20, C.2) and TrustListType (7.8.2.1) Methods */

static UA_StatusCode
openTrustListFile(UA_Variant *output) {
    cm.open++;
    if(cm.openHandle != 0)
        return UA_STATUSCODE_BADNOTWRITABLE; /* already open */
    cm.openHandle = ++cm.nextHandle;
    cm.position = 0;
    return UA_Variant_setScalarCopy(output, &cm.openHandle,
                                    &UA_TYPES[UA_TYPES_UINT32]);
}

static UA_StatusCode
checkFileHandle(const UA_Variant *v) {
    if(!isScalar(v, &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(cm.openHandle == 0 || *(const UA_UInt32*)v->data != cm.openHandle)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

/* Open(Mode) -> FileHandle; only reading is supported */
MOCK_METHOD(mockOpen) {
    if(!isScalar(&input[0], &UA_TYPES[UA_TYPES_BYTE]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if((*(const UA_Byte*)input[0].data & 0x01) == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return openTrustListFile(&output[0]);
}

/* OpenWithMasks(Masks) -> FileHandle */
MOCK_METHOD(mockOpenWithMasks) {
    if(!isScalar(&input[0], &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return openTrustListFile(&output[0]);
}

/* Read(FileHandle, Length) -> Data */
MOCK_METHOD(mockRead) {
    cm.read++;
    UA_StatusCode res = checkFileHandle(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[1], &UA_TYPES[UA_TYPES_INT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Int32 length = *(const UA_Int32*)input[1].data;
    if(length < 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t remaining = cm.trustListFile.length - cm.position;
    size_t chunk = ((size_t)length < remaining) ? (size_t)length : remaining;
    UA_ByteString data = {chunk, cm.trustListFile.data + cm.position};
    cm.position += chunk;
    return UA_Variant_setScalarCopy(&output[0], &data,
                                    &UA_TYPES[UA_TYPES_BYTESTRING]);
}

/* Close(FileHandle) */
MOCK_METHOD(mockClose) {
    cm.close++;
    UA_StatusCode res = checkFileHandle(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    cm.openHandle = 0;
    return UA_STATUSCODE_GOOD;
}

/* GetPosition(FileHandle) -> Position */
MOCK_METHOD(mockGetPosition) {
    UA_StatusCode res = checkFileHandle(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_UInt64 position = (UA_UInt64)cm.position;
    return UA_Variant_setScalarCopy(&output[0], &position,
                                    &UA_TYPES[UA_TYPES_UINT64]);
}

/* SetPosition(FileHandle, Position) */
MOCK_METHOD(mockSetPosition) {
    UA_StatusCode res = checkFileHandle(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[1], &UA_TYPES[UA_TYPES_UINT64]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt64 position = *(const UA_UInt64*)input[1].data;
    if(position > cm.trustListFile.length)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    cm.position = (size_t)position;
    return UA_STATUSCODE_GOOD;
}

/* Address space of the mock */

static UA_Argument
argument(const char *name, const UA_DataType *type, UA_Int32 valueRank) {
    UA_Argument a;
    UA_Argument_init(&a);
    a.name = UA_STRING((char*)(uintptr_t)name);
    a.dataType = type->typeId;
    a.valueRank = valueRank;
    return a;
}

static void
addDirectoryMethod(UA_UInt32 id, const char *name, UA_MethodCallback callback,
                   size_t inputSize, const UA_Argument *input,
                   size_t outputSize, const UA_Argument *output) {
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)name);
    attr.executable = true;
    attr.userExecutable = true;
    UA_StatusCode res = UA_Server_addMethodNode(
        cm.server, UA_NODEID_NUMERIC(cm.ns, id),
        UA_NODEID_NUMERIC(cm.ns, GDSID_DIRECTORY),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(cm.ns, (char*)(uintptr_t)name), attr, callback,
        inputSize, input, outputSize, output, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
addObject(UA_UInt32 id, UA_NodeId parent, UA_UInt16 browseNameNs,
          const char *name, UA_UInt32 typeDefinition) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)name);
    UA_StatusCode res = UA_Server_addObjectNode(
        cm.server, UA_NODEID_NUMERIC(cm.ns, id), parent,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(browseNameNs, (char*)(uintptr_t)name),
        UA_NODEID_NUMERIC(0, typeDefinition), attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

/* Find a child of the instantiated TrustList object by BrowseName */
static UA_NodeId
findChild(UA_NodeId parent, const char *name) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parent;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(cm.server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId found = UA_NODEID_NULL;
    UA_String needle = UA_STRING((char*)(uintptr_t)name);
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(!UA_String_equal(&br.references[i].browseName.name, &needle))
            continue;
        UA_NodeId_copy(&br.references[i].nodeId.nodeId, &found);
        break;
    }
    UA_BrowseResult_clear(&br);
    ck_assert_msg(!UA_NodeId_isNull(&found), "TrustList child %s not found", name);
    return found;
}

static void
setTrustListMethod(const char *name, UA_MethodCallback callback) {
    UA_NodeId methodId = findChild(cm.trustListId, name);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(cm.server, methodId, callback),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&methodId);
}

/* Replace the served TrustList. The LastUpdateTime is set to the current time
 * so that requesters recognize the change (7.8.2.1). */
static void
setServedTrustList(const UA_ByteString *certificates, size_t size) {
    lockCm();
    UA_TrustListDataType_clear(&cm.trustList);
    cm.trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    UA_StatusCode res = UA_Array_copy(certificates, size,
                                      (void**)&cm.trustList.trustedCertificates,
                                      &UA_TYPES[UA_TYPES_BYTESTRING]);
    cm.trustList.trustedCertificatesSize = size;
    UA_ByteString_clear(&cm.trustListFile);
    res |= UA_encodeBinary(&cm.trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                           &cm.trustListFile, NULL);
    UA_UInt64 fileSize = (UA_UInt64)cm.trustListFile.length;
    unlockCm();
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Node values are written without holding cmMutex. The server lock is
     * taken inside and might be held by the CertificateManager thread while
     * it waits for cmMutex in a Method callback. */
    UA_Variant v;
    UA_Variant_setScalar(&v, &fileSize, &UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(UA_Server_writeValue(cm.server, cm.trustListSizeId, v),
                      UA_STATUSCODE_GOOD);
    UA_DateTime now = UA_DateTime_now();
    UA_Variant_setScalar(&v, &now, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert_uint_eq(UA_Server_writeValue(cm.server, cm.trustListLastUpdateTimeId, v),
                      UA_STATUSCODE_GOOD);
}

/* A server with all security policies that trusts one certificate. Unlike the
 * unit test helpers, the EventLoop keeps the real clock. */
static UA_Server *
newServer(UA_UInt16 port, const UA_ByteString *certificate,
          const UA_ByteString *privateKey, const UA_ByteString *trusted,
          const char *applicationUri) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_INFO);
    ck_assert_uint_eq(UA_ServerConfig_setDefaultWithSecurityPolicies(
                          &config, port, certificate, privateKey, trusted, 1,
                          NULL, 0, NULL, 0),
                      UA_STATUSCODE_GOOD);
    config.tcpReuseAddr = true;
    UA_String_clear(&config.applicationDescription.applicationUri);
    config.applicationDescription.applicationUri = UA_STRING_ALLOC(applicationUri);
    UA_Server *s = UA_Server_newWithConfig(&config);
    ck_assert_ptr_nonnull(s);
    return s;
}

THREAD_CALLBACK(certificateManagerLoop) {
    while(cmRunning)
        UA_Server_run_iterate(cm.server, true);
    return 0;
}

static void
setupCertificateManager(void) {
    memset(&cm, 0, sizeof(cm));
    ck_assert(MUTEX_INIT(cmMutex));

    /* The CertificateManager trusts the requester */
    cm.server = newServer(GDS_PORT, &gdsCert, &gdsKey, &requesterCert, GDS_URI);

    cm.ns = UA_Server_addNamespace(cm.server, GDS_NS_URI);
    cm.applicationId = UA_NODEID_NUMERIC(cm.ns, GDSID_APPLICATION);
    cm.groupId = UA_NODEID_NUMERIC(
        cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    cm.trustListId = UA_NODEID_NUMERIC(
        cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    cm.updateRequired = true;
    cm.finishMode = FINISH_APPROVE;
    cm.issuedCertificate = requesterCert;

    /* Directory object (CertificateDirectoryType, 7.9.2) */
    addObject(GDSID_DIRECTORY, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
              cm.ns, "Directory", UA_NS0ID_BASEOBJECTTYPE);
    addObject(GDSID_DIRECTORY_CERTIFICATEGROUPS,
              UA_NODEID_NUMERIC(cm.ns, GDSID_DIRECTORY), 0,
              "CertificateGroups", UA_NS0ID_FOLDERTYPE);
    addObject(GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP,
              UA_NODEID_NUMERIC(cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS), 0,
              "DefaultApplicationGroup", UA_NS0ID_BASEOBJECTTYPE);
    addObject(GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
              cm.groupId, 0, "TrustList", UA_NS0ID_TRUSTLISTTYPE);

    const UA_DataType *nodeId = &UA_TYPES[UA_TYPES_NODEID];
    const UA_DataType *byteString = &UA_TYPES[UA_TYPES_BYTESTRING];
    const UA_DataType *boolean = &UA_TYPES[UA_TYPES_BOOLEAN];
    const UA_DataType *string = &UA_TYPES[UA_TYPES_STRING];

    UA_Argument in[4], out[3];

    in[0] = argument("ApplicationUri", string, UA_VALUERANK_SCALAR);
    addDirectoryMethod(GDSID_DIRECTORY_FINDAPPLICATIONS, "FindApplications",
                       mockFindApplications, 1, in, 0, NULL);
    addDirectoryMethod(GDSID_DIRECTORY_REGISTERAPPLICATION, "RegisterApplication",
                       mockRegisterApplication, 0, NULL, 0, NULL);

    in[0] = argument("ApplicationId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("CertificateGroupIds", nodeId, UA_VALUERANK_ONE_DIMENSION);
    addDirectoryMethod(GDSID_DIRECTORY_GETCERTIFICATEGROUPS, "GetCertificateGroups",
                       mockGetCertificateGroups, 1, in, 1, out);

    in[1] = argument("CertificateGroupId", nodeId, UA_VALUERANK_SCALAR);
    in[2] = argument("CertificateTypeId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("UpdateRequired", boolean, UA_VALUERANK_SCALAR);
    addDirectoryMethod(GDSID_DIRECTORY_GETCERTIFICATESTATUS, "GetCertificateStatus",
                       mockGetCertificateStatus, 3, in, 1, out);

    in[3] = argument("CertificateRequest", byteString, UA_VALUERANK_SCALAR);
    out[0] = argument("RequestId", nodeId, UA_VALUERANK_SCALAR);
    addDirectoryMethod(GDSID_DIRECTORY_STARTSIGNINGREQUEST, "StartSigningRequest",
                       mockStartSigningRequest, 4, in, 1, out);
    addDirectoryMethod(GDSID_DIRECTORY_STARTNEWKEYPAIRREQUEST, "StartNewKeyPairRequest",
                       mockStartNewKeyPairRequest, 0, NULL, 0, NULL);

    in[1] = argument("RequestId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("Certificate", byteString, UA_VALUERANK_SCALAR);
    out[1] = argument("PrivateKey", byteString, UA_VALUERANK_SCALAR);
    out[2] = argument("IssuerCertificates", byteString, UA_VALUERANK_ONE_DIMENSION);
    addDirectoryMethod(GDSID_DIRECTORY_FINISHREQUEST, "FinishRequest",
                       mockFinishRequest, 2, in, 3, out);

    in[1] = argument("CertificateGroupId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("TrustListId", nodeId, UA_VALUERANK_SCALAR);
    addDirectoryMethod(GDSID_DIRECTORY_GETTRUSTLIST, "GetTrustList",
                       mockGetTrustList, 2, in, 1, out);

    /* The TrustList object was instantiated from TrustListType */
    cm.trustListSizeId = findChild(cm.trustListId, "Size");
    cm.trustListLastUpdateTimeId = findChild(cm.trustListId, "LastUpdateTime");
    setTrustListMethod("Open", mockOpen);
    setTrustListMethod("OpenWithMasks", mockOpenWithMasks);
    setTrustListMethod("Read", mockRead);
    setTrustListMethod("Close", mockClose);
    setTrustListMethod("GetPosition", mockGetPosition);
    setTrustListMethod("SetPosition", mockSetPosition);

    UA_ByteString served[2] = {gdsCert, requesterCert};
    setServedTrustList(served, 2);

    ck_assert_uint_eq(UA_Server_run_startup(cm.server), UA_STATUSCODE_GOOD);
    cmRunning = true;
    THREAD_CREATE(cmThread, certificateManagerLoop);
}

static void
teardownCertificateManager(void) {
    cmRunning = false;
    THREAD_JOIN(cmThread);
    ck_assert_uint_eq(UA_Server_run_shutdown(cm.server), UA_STATUSCODE_GOOD);
    UA_Server_delete(cm.server);
    UA_NodeId_clear(&cm.trustListSizeId);
    UA_NodeId_clear(&cm.trustListLastUpdateTimeId);
    UA_TrustListDataType_clear(&cm.trustList);
    UA_ByteString_clear(&cm.trustListFile);
    UA_ByteString_clear(&cm.lastCsr);
    ck_assert(MUTEX_DESTROY(cmMutex));
    memset(&cm, 0, sizeof(cm));
}

/******************************/
/* Requester                  */
/******************************/

static UA_Server *server;
static UA_GDSPullRequester *pull;
static UA_Boolean serverStarted;

typedef struct {
    UA_GDSPullRequesterNotification type;
    UA_KeyValueMap payload;
} Notification;

static Notification notifications[MAX_NOTIFICATIONS];
static size_t notificationsSize;

static const char *
notificationName(UA_GDSPullRequesterNotification type) {
    switch(type) {
    case UA_GDSPULL_CYCLE_STARTED: return "CYCLE_STARTED";
    case UA_GDSPULL_CYCLE_FINISHED: return "CYCLE_FINISHED";
    case UA_GDSPULL_REQUEST_ADDED: return "REQUEST_ADDED";
    case UA_GDSPULL_REQUEST_FAILED: return "REQUEST_FAILED";
    case UA_GDSPULL_REQUEST_DROPPED: return "REQUEST_DROPPED";
    case UA_GDSPULL_REQUEST_FINISHED: return "REQUEST_FINISHED";
    case UA_GDSPULL_CERTIFICATE_INSTALLED: return "CERTIFICATE_INSTALLED";
    case UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED: return "CERTIFICATE_INSTALLATION_FAILED";
    case UA_GDSPULL_TRUSTLIST_UPDATED: return "TRUSTLIST_UPDATED";
    case UA_GDSPULL_TRUSTLIST_FAILED: return "TRUSTLIST_FAILED";
    default: return "UNKNOWN";
    }
}

static void
onNotification(UA_GDSPullRequester *requester, void *context,
               UA_GDSPullRequesterNotification type,
               const UA_KeyValueMap payload) {
    ck_assert_ptr_eq(requester, context);
    ck_assert_uint_lt(notificationsSize, MAX_NOTIFICATIONS);
    Notification *n = &notifications[notificationsSize++];
    n->type = type;
    ck_assert_uint_eq(UA_KeyValueMap_copy(&payload, &n->payload), UA_STATUSCODE_GOOD);
    printf("GDS Pull notification %s\n", notificationName(type));
}

static void
clearNotifications(void) {
    for(size_t i = 0; i < notificationsSize; i++)
        UA_KeyValueMap_clear(&notifications[i].payload);
    notificationsSize = 0;
}

static size_t
countNotifications(UA_GDSPullRequesterNotification type) {
    size_t count = 0;
    for(size_t i = 0; i < notificationsSize; i++)
        count += (notifications[i].type == type);
    return count;
}

/* Position of the n-th (1-based) notification of the type. The notification
 * must exist. */
static size_t
positionOf(UA_GDSPullRequesterNotification type, size_t n) {
    for(size_t i = 0; i < notificationsSize; i++) {
        if(notifications[i].type != type)
            continue;
        if(--n == 0)
            return i;
    }
    ck_abort_msg("Notification %s not found", notificationName(type));
    return 0;
}

static const UA_NodeId *
payloadNodeId(size_t position, const char *key) {
    const UA_NodeId *id = (const UA_NodeId*)UA_KeyValueMap_getScalar(
        &notifications[position].payload, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key),
        &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_msg(id != NULL, "Notification %s has no NodeId payload %s",
                  notificationName(notifications[position].type), key);
    return id;
}

static UA_StatusCode
payloadStatusCode(size_t position) {
    const UA_StatusCode *sc = (const UA_StatusCode*)UA_KeyValueMap_getScalar(
        &notifications[position].payload, UA_QUALIFIEDNAME(0, "status-code"),
        &UA_TYPES[UA_TYPES_STATUSCODE]);
    ck_assert_msg(sc != NULL, "Notification %s has no status-code payload",
                  notificationName(notifications[position].type));
    return *sc;
}

/* Assert that the notification refers to the request handed out by the
 * CertificateManager for the local DefaultApplicationGroup */
static void
assertRequestPayload(size_t position, const UA_NodeId *requestId,
                     const UA_NodeId *certificateType) {
    ck_assert(UA_NodeId_equal(payloadNodeId(position, "request-id"), requestId));
    ck_assert(UA_NodeId_equal(payloadNodeId(position, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert(UA_NodeId_equal(payloadNodeId(position, "certificate-type-id"),
                              certificateType));
}

/* One iteration of the requester. The server waits for network activity (the
 * CertificateManager answers from its own thread) or for the next timer. */
static void
iterate(void) {
    UA_Server_run_iterate(server, true);
}

static UA_DateTime
deadline(void) {
    return UA_DateTime_nowMonotonic() + WAIT_TIMEOUT_MS * UA_DATETIME_MSEC;
}

/* Iterate until the n-th notification of the given type was seen. Returns
 * false when the time budget is used up. */
static UA_Boolean
runUntil(UA_GDSPullRequesterNotification type, size_t n) {
    UA_DateTime end = deadline();
    while(countNotifications(type) < n && UA_DateTime_nowMonotonic() < end)
        iterate();
    return countNotifications(type) >= n;
}

static UA_GDSPullRequester *
newRequester(const char *endpointUrl, size_t pendingSize,
             const UA_NodeId *pendingRequestIds) {
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_String url = UA_STRING((char*)(uintptr_t)endpointUrl);
    UA_Boolean createPrivateKey = false;
    UA_UInt32 cycleInterval = CYCLE_INTERVAL_MS;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "endpoint-url"),
                                    &url, &UA_TYPES[UA_TYPES_STRING]);
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "client-certificate"),
                                    &requesterCert, &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "client-key"),
                                    &requesterKey, &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "application-id"),
                                    &cm.applicationId, &UA_TYPES[UA_TYPES_NODEID]);
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "create-private-key"),
                                    &createPrivateKey, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res |= UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "cycle-interval"),
                                    &cycleInterval, &UA_TYPES[UA_TYPES_UINT32]);

    /* The local DefaultApplicationGroup is managed by the DefaultApplicationGroup
     * of the CertificateManager */
    UA_Variant v;
    UA_NodeId localGroup = defaultApplicationGroup;
    UA_Variant_setArray(&v, &localGroup, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res |= UA_KeyValueMap_set(&params, UA_QUALIFIEDNAME(0, "mappings-local-group-id"), &v);
    UA_Variant_setArray(&v, &cm.groupId, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res |= UA_KeyValueMap_set(&params, UA_QUALIFIEDNAME(0, "mappings-remote-group-id"), &v);

    /* Outstanding signing requests of a previous run */
    if(pendingSize > 0) {
        UA_NodeId groups[MAX_REQUESTS], types[MAX_REQUESTS];
        for(size_t i = 0; i < pendingSize; i++) {
            groups[i] = defaultApplicationGroup;
            types[i] = rsaSha256CertificateType;
        }
        UA_Variant_setArray(&v, (void*)(uintptr_t)pendingRequestIds, pendingSize,
                            &UA_TYPES[UA_TYPES_NODEID]);
        res |= UA_KeyValueMap_set(&params, UA_QUALIFIEDNAME(0, "pending-request-id"), &v);
        UA_Variant_setArray(&v, groups, pendingSize, &UA_TYPES[UA_TYPES_NODEID]);
        res |= UA_KeyValueMap_set(&params, UA_QUALIFIEDNAME(0, "pending-certificate-group-id"), &v);
        UA_Variant_setArray(&v, types, pendingSize, &UA_TYPES[UA_TYPES_NODEID]);
        res |= UA_KeyValueMap_set(&params, UA_QUALIFIEDNAME(0, "pending-certificate-type-id"), &v);
    }
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_GDSPullRequester *requester = UA_GDSPull_new(params);
    UA_KeyValueMap_clear(&params);
    ck_assert_ptr_nonnull(requester);
    UA_GDSPull_setNotificationCallback(requester, onNotification, requester);
    return requester;
}

/* Replace the requester of the fixture before the server is started */
static void
replaceRequester(const char *endpointUrl, size_t pendingSize,
                 const UA_NodeId *pendingRequestIds) {
    ck_assert_uint_eq(UA_Server_removeDriver(server, &pull->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pull->drv.free(&pull->drv), UA_STATUSCODE_GOOD);
    pull = newRequester(endpointUrl, pendingSize, pendingRequestIds);
    ck_assert_uint_eq(UA_Server_addDriver(server, &pull->drv), UA_STATUSCODE_GOOD);
}

static void
setup(void) {
    clearNotifications();
    setupCertificateManager();

    /* The requester trusts the CertificateManager */
    server = newServer(REQUESTER_PORT, &requesterCert, &requesterKey, &gdsCert,
                       REQUESTER_URI);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(UA_NodeId_equal(&config->secureChannelPKI.certificateGroupId,
                              &defaultApplicationGroup));

    /* The GDS namespace is the first additional namespace on both servers, so
     * the NodeIds of the CertificateManager can be passed to the driver as
     * they are. */
    UA_UInt16 gdsNs = UA_Server_addNamespace(server, GDS_NS_URI);
    ck_assert_uint_eq(gdsNs, cm.ns);

    pull = newRequester(GDS_ENDPOINT, 0, NULL);
    ck_assert_uint_eq(UA_Server_addDriver(server, &pull->drv), UA_STATUSCODE_GOOD);
    serverStarted = false;
}

static void
startup(void) {
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    serverStarted = true;
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
}

/* Stopping the driver is asynchronous. The connection to the
 * CertificateManager is closed in the process. */
static void
stopRequester(void) {
    pull->drv.stop(&pull->drv);
    UA_DateTime end = deadline();
    while(pull->drv.state != UA_LIFECYCLESTATE_STOPPED && UA_DateTime_nowMonotonic() < end)
        iterate();
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);
}

static void
shutdownRequester(void) {
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);
}

static void
teardown(void) {
    if(serverStarted) {
        if(pull && pull->drv.state != UA_LIFECYCLESTATE_STOPPED)
            stopRequester();
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    UA_Server_delete(server);
    teardownCertificateManager();
    clearNotifications();
}

static UA_Boolean
listContains(const UA_ByteString *list, size_t size, const UA_ByteString *item) {
    for(size_t i = 0; i < size; i++) {
        if(UA_ByteString_equal(&list[i], item))
            return true;
    }
    return false;
}

/* The trusted certificates of the local DefaultApplicationGroup are the ones
 * served by the CertificateManager */
static void
assertLocalTrustListEquals(const UA_TrustListDataType *expected) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_TrustListDataType local;
    UA_TrustListDataType_init(&local);
    ck_assert_uint_eq(config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &local),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(local.trustedCertificatesSize, expected->trustedCertificatesSize);
    for(size_t i = 0; i < expected->trustedCertificatesSize; i++)
        ck_assert(listContains(local.trustedCertificates, local.trustedCertificatesSize,
                               &expected->trustedCertificates[i]));
    UA_TrustListDataType_clear(&local);
}

static void
assertLocalCertificateIs(const UA_ByteString *certificate) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t checked = 0;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &config->securityPolicies[i];
        if(sp->localCertificate.length == 0)
            continue;
        ck_assert(UA_ByteString_equal(&sp->localCertificate, certificate));
        checked++;
    }
    ck_assert_uint_gt(checked, 0);
}

/******************************/
/* Tests                      */
/******************************/

/* 7.6: The whole workflow within one cycle. GetCertificateStatus reports an
 * update, the CSR is signed for the existing key, FinishRequest returns the
 * certificate, and the TrustList is read from the CertificateManager. */
START_TEST(issueCertificateAndUpdateTrustList) {
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FAILED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_FAILED), 0);

    /* The notifications follow the order of the workflow: certificates before
     * the TrustList, everything within one cycle */
    size_t added = positionOf(UA_GDSPULL_REQUEST_ADDED, 1);
    size_t finished = positionOf(UA_GDSPULL_REQUEST_FINISHED, 1);
    size_t installed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    size_t trustList = positionOf(UA_GDSPULL_TRUSTLIST_UPDATED, 1);
    ck_assert_uint_eq(positionOf(UA_GDSPULL_CYCLE_STARTED, 1), 0);
    ck_assert_uint_lt(added, finished);
    ck_assert_uint_lt(added, installed);
    ck_assert_uint_lt(finished, trustList);
    ck_assert_uint_lt(installed, trustList);
    ck_assert_uint_eq(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), notificationsSize - 1);

    /* The payloads identify the request and the local group */
    assertRequestPayload(added, &c.lastRequestId, &c.lastCertificateType);
    assertRequestPayload(finished, &c.lastRequestId, &c.lastCertificateType);
    ck_assert(UA_NodeId_equal(payloadNodeId(installed, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert_uint_eq(payloadStatusCode(installed), UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(payloadNodeId(trustList, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert_uint_eq(payloadStatusCode(trustList), UA_STATUSCODE_GOOD);

    /* The CertificateManager saw the calls prescribed by the workflow */
    ck_assert_uint_eq(c.findApplications, 0);
    ck_assert_uint_eq(c.registerApplication, 0);
    ck_assert_uint_eq(c.startNewKeyPairRequest, 0);
    ck_assert_uint_gt(c.getCertificateStatus, 0);
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.getTrustList, 1);
    ck_assert_uint_eq(c.open, 1);
    ck_assert_uint_gt(c.read, 0);
    ck_assert_uint_eq(c.close, 1);
    ck_assert_uint_eq(c.requestsSize, 0);

    /* The CSR was created for the existing private key */
    ck_assert_uint_eq(UA_CertificateUtils_comparePublicKeys(&c.lastCsr, &requesterCert),
                      UA_STATUSCODE_GOOD);

    assertLocalCertificateIs(&requesterCert);
    assertLocalTrustListEquals(&c.trustList);
} END_TEST

/* 7.6: GetCertificateStatus returns UpdateRequired=false. No signing request
 * is started, but the TrustList is still retrieved. */
START_TEST(noUpdateRequiredSkipsSigningRequest) {
    setUpdateRequired(false);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    ck_assert_uint_gt(c.getCertificateStatus, 0);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.finishRequest, 0);
    ck_assert_uint_eq(c.getTrustList, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);
    assertLocalTrustListEquals(&c.trustList);
} END_TEST

/* 7.6: FinishRequest returns Bad_NothingToDo. The RequestId is kept and
 * FinishRequest is called directly in the next cycle without a new
 * StartSigningRequest. */
START_TEST(pendingRequestIsFinishedInLaterCycle) {
    setFinishMode(FINISH_PENDING);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_gt(c.finishRequest, 0);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    size_t added = positionOf(UA_GDSPULL_REQUEST_ADDED, 1);
    assertRequestPayload(added, &c.lastRequestId, &c.lastCertificateType);

    /* The CertificateManager approves the request in the meantime */
    setFinishMode(FINISH_APPROVE);
    size_t finishRequestsBefore = c.finishRequest;
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_gt(c.finishRequest, finishRequestsBefore);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    size_t finished = positionOf(UA_GDSPULL_REQUEST_FINISHED, 1);
    ck_assert_uint_lt(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), finished);
    assertRequestPayload(finished, &c.lastRequestId, &c.lastCertificateType);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.6: FinishRequest returns a Bad result other than Bad_NothingToDo. The
 * request is dropped and a new request is sent in the next cycle. */
START_TEST(rejectedRequestIsReplacedInNextCycle) {
    setFinishMode(FINISH_REJECT);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId rejectedRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);

    setFinishMode(FINISH_APPROVE);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 2);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert(!UA_NodeId_equal(&c.lastRequestId, &rejectedRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_DROPPED, 1),
                         &rejectedRequest, &c.lastCertificateType);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_ADDED, 2),
                         &c.lastRequestId, &c.lastCertificateType);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &c.lastRequestId, &c.lastCertificateType);
} END_TEST

/* 7.6: A pending request supplied at creation is finished directly with
 * FinishRequest. No new signing request is started for it. */
START_TEST(restoredPendingRequestIsFinishedWithoutNewRequest) {
    lockCm();
    UA_NodeId pendingRequest = addRequest();
    unlockCm();
    ck_assert(!UA_NodeId_isNull(&pendingRequest));
    replaceRequester(GDS_ENDPOINT, 1, &pendingRequest);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &pendingRequest, &rsaSha256CertificateType);
    assertLocalCertificateIs(&requesterCert);
} END_TEST

/* Without a reachable CertificateManager the cycle ends without any request
 * and the driver keeps running until the server shuts down. */
START_TEST(unreachableCertificateManager) {
    replaceRequester(UNUSED_ENDPOINT, 0, NULL);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_STARTED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_FINISHED), 1);
    ck_assert_uint_eq(notificationsSize, 2);
    ck_assert_uint_eq(snapshot().getCertificateStatus, 0);

    shutdownRequester();
} END_TEST

/* The driver can be stopped while a cycle is active. The workflow ends, the
 * stopped driver can be removed, and a new requester takes its place. */
START_TEST(stopDuringCycleAndReplaceRequester) {
    setFinishMode(FINISH_PENDING);
    startup();
    ck_assert(runUntil(UA_GDSPULL_REQUEST_ADDED, 1));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);

    stopRequester();
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_FINISHED), 1);
    ck_assert_uint_eq(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), notificationsSize - 1);
    shutdownRequester();
    ck_assert_uint_eq(UA_Server_removeDriver(server, &pull->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pull->drv.free(&pull->drv), UA_STATUSCODE_GOOD);
    pull = NULL;

    /* The CertificateManager forgot about the request in the meantime */
    clearNotifications();
    lockCm();
    cm.finishMode = FINISH_APPROVE;
    cm.requestsSize = 0;
    unlockCm();

    pull = newRequester(GDS_ENDPOINT, 0, NULL);
    ck_assert_uint_eq(UA_Server_addDriver(server, &pull->drv), UA_STATUSCODE_GOOD);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.6, 7.8.2.1: The next cycle checks the certificate status again. The
 * LastUpdateTime of the TrustList has not changed, so the TrustList is not
 * downloaded again. */
START_TEST(unchangedTrustListIsNotDownloadedAgain) {
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager first = snapshot();
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);

    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    MockCertificateManager second = snapshot();
    ck_assert_uint_gt(second.getCertificateStatus, first.getCertificateStatus);
    ck_assert_uint_eq(second.startSigningRequest, 1);
    ck_assert_uint_eq(second.getTrustList, 2);
    ck_assert_uint_eq(second.open, first.open);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_STARTED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.6, 7.8.2.1: The TrustList of the CertificateManager changed. The next
 * cycle downloads it and replaces the local TrustList. */
START_TEST(changedTrustListIsDownloadedAgain) {
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);

    UA_ByteString served[3] = {gdsCert, requesterCert, foreignCert};
    setServedTrustList(served, 3);

    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.open, 2);
    ck_assert_uint_eq(c.close, 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 2);
    ck_assert_uint_eq(payloadStatusCode(positionOf(UA_GDSPULL_TRUSTLIST_UPDATED, 2)),
                      UA_STATUSCODE_GOOD);
    assertLocalTrustListEquals(&c.trustList);
} END_TEST

/* The issued certificate does not match the private key of the CSR. It must
 * not replace the application instance certificate. */
START_TEST(certificateNotMatchingKeyIsNotInstalled) {
    setIssuedCertificate(foreignCert);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 1);
    size_t failed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED, 1);
    ck_assert(UA_NodeId_equal(payloadNodeId(failed, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert_uint_ne(payloadStatusCode(failed), UA_STATUSCODE_GOOD);
    assertLocalCertificateIs(&requesterCert);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/******************************/
/* Suite                      */
/******************************/

static void
createCertificate(const char *commonName, const char *uri,
                  UA_ByteString *privateKey, UA_ByteString *certificate) {
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"), UA_STRING_STATIC("O=open62541"),
                            UA_STRING_NULL};
    subject[2] = UA_STRING((char*)(uintptr_t)commonName);
    UA_String subjectAltName[2] = {UA_STRING_STATIC("DNS:localhost"), UA_STRING_NULL};
    subjectAltName[1] = UA_STRING((char*)(uintptr_t)uri);
    UA_UInt16 keySizeBits = 2048;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             &keySizeBits, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode res = UA_CreateCertificate(
        UA_Log_Stdout, subject, 3, subjectAltName, 2, UA_CERTIFICATEFORMAT_DER,
        &params, privateKey, certificate);
    UA_KeyValueMap_clear(&params);
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Creating the certificate %s failed with %s\n",
                commonName, UA_StatusCode_name(res));
        exit(EXIT_FAILURE);
    }
}

int
main(void) {
    createCertificate("CN=GDS Pull Requester", "URI:" REQUESTER_URI,
                      &requesterKey, &requesterCert);
    createCertificate("CN=Mock CertificateManager", "URI:" GDS_URI,
                      &gdsKey, &gdsCert);
    createCertificate("CN=Foreign Application", "URI:urn:open62541.test.foreign",
                      &foreignKey, &foreignCert);

    Suite *suite = suite_create("GDS Pull Requester lifecycle");
    TCase *tc = tcase_create("lifecycle");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, issueCertificateAndUpdateTrustList);
    tcase_add_test(tc, noUpdateRequiredSkipsSigningRequest);
    tcase_add_test(tc, pendingRequestIsFinishedInLaterCycle);
    tcase_add_test(tc, rejectedRequestIsReplacedInNextCycle);
    tcase_add_test(tc, restoredPendingRequestIsFinishedWithoutNewRequest);
    tcase_add_test(tc, unreachableCertificateManager);
    tcase_add_test(tc, stopDuringCycleAndReplaceRequester);
    tcase_add_test(tc, unchangedTrustListIsNotDownloadedAgain);
    tcase_add_test(tc, changedTrustListIsDownloadedAgain);
    tcase_add_test(tc, certificateNotMatchingKeyIsNotInstalled);
    suite_add_tcase(suite, tc);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    UA_ByteString_clear(&requesterCert);
    UA_ByteString_clear(&requesterKey);
    UA_ByteString_clear(&gdsCert);
    UA_ByteString_clear(&gdsKey);
    UA_ByteString_clear(&foreignCert);
    UA_ByteString_clear(&foreignKey);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
