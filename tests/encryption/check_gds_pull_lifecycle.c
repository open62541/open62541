/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Lifecycle tests for the GDS Pull Requester driver. */

#include <open62541/driver/gds_pull.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include "thread_wrapper.h"

#ifndef UA_ARCHITECTURE_WIN32
#include <time.h>
#endif

/* A second certificate for an existing key needs the crypto library */
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/evp.h>
#include <openssl/x509.h>
#define CAN_RENEW_CERTIFICATE
#elif defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x03050000
#include "crypto/mbedtls/securitypolicy_mbedtls_compat.h"
#define CAN_RENEW_CERTIFICATE
#endif
#endif

#define REQUESTER_PORT 14840
#define GDS_PORT 14841
#define GDS_ENDPOINT "opc.tcp://localhost:14841"
#define UNUSED_ENDPOINT "opc.tcp://localhost:14842"

#define REQUESTER_URI "urn:open62541.test.gds-pull-requester"
#define AES128_POLICY_URI \
    "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep"
#define GDS_URI "urn:open62541.test.certificate-manager"

/* Well-known NodeIds of the GDS namespace (Opc.Ua.Gds.NodeIds.csv) */
#define GDS_NS_URI "http://opcfoundation.org/UA/GDS/"
#define GDSID_DIRECTORY 141
#define GDSID_DIRECTORY_FINDAPPLICATIONS 143
#define GDSID_DIRECTORY_REGISTERAPPLICATION 146
#define GDSID_DIRECTORY_STARTNEWKEYPAIRREQUEST 154
#define GDSID_DIRECTORY_STARTSIGNINGREQUEST 157
#define GDSID_DIRECTORY_FINISHREQUEST 163
#define GDSID_DIRECTORY_GETCERTIFICATES 174
#define GDSID_DIRECTORY_GETTRUSTLIST 204
#define GDSID_DIRECTORY_GETCERTIFICATESTATUS 225
#define GDSID_DIRECTORY_GETCERTIFICATEGROUPS 508
#define GDSID_DIRECTORY_CERTIFICATEGROUPS 614
#define GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP 615
#define GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST 616
#define GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP 683

/* Identifiers handed out by the mock CertificateManager */
#define GDSID_APPLICATION 1000
#define GDSID_FIRST_REQUEST 2000

/* Interval of the driver cycles and the real-time budget for waiting on a
 * notification */
#define CYCLE_INTERVAL_MS 500
#define WAIT_TIMEOUT_MS 60000

#define MAX_NOTIFICATIONS 128
#define MAX_REQUESTS 8
#define MAX_FINISH_CALLS 32

/* Certificates are created once for the whole suite */
static UA_ByteString requesterCert, requesterKey;
static UA_ByteString gdsCert, gdsKey;
static UA_ByteString foreignCert, foreignKey;
#ifdef CAN_RENEW_CERTIFICATE
/* Another certificate for requesterKey */
static UA_ByteString renewedCert;
#endif
/* A new key pair for the requester. Issued by StartNewKeyPairRequest. */
static UA_ByteString newKeyPairCert, newKeyPairKey;
/* Another new key pair, issued for the second CertificateGroup */
static UA_ByteString userTokenGroupCert, userTokenGroupKey;

static const UA_NodeId defaultApplicationGroup = {
    0, UA_NODEIDTYPE_NUMERIC,
    {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP}};
static const UA_NodeId defaultUserTokenGroup = {
    0, UA_NODEIDTYPE_NUMERIC,
    {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP}};
static const UA_NodeId rsaSha256CertificateType = {
    0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE}};

/******************************/
/* Mock CertificateManager    */
/******************************/

typedef enum {
    FINISH_APPROVE, /* FinishRequest returns the issued certificate */
    FINISH_PENDING, /* FinishRequest returns Bad_NothingToDo */
    FINISH_REJECT,  /* FinishRequest returns Bad_RequestNotAllowed */
    FINISH_DENIED,  /* FinishRequest returns Bad_UserAccessDenied and keeps the
                     * request, as a CertificateManager does that enforces
                     * 7.9.5 when the caller's certificate differs from the
                     * one used to start the request */
    /* FinishRequest issues the certificate like FINISH_APPROVE, but the
     * SecureChannel is closed before the response is sent. Then the mode
     * falls back to FINISH_APPROVE. */
    FINISH_APPROVE_LOST
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
    /* Whether the application belongs to the CertificateGroup. An application
     * that does not belong to it joins with a successful StartSigningRequest
     * (7.9.3), as in a CertificateManager that assigns groups on request. */
    UA_Boolean assigned;
    FinishMode finishMode;
    UA_ByteString issuedCertificate; /* not owned */
    UA_ByteString issuedPrivateKey; /* not owned, returned by FinishRequest */
    /* GetCertificates returns the issued certificate once one was issued */
    UA_Boolean certificateIssued;
    /* Issued requests are not erased and FinishRequest answers them again,
     * but the private key is handed out only once */
    UA_Boolean keepIssuedRequests;
    size_t keyDeliveredSize;
    UA_NodeId keyDelivered[MAX_REQUESTS];

    /* The response to the FinishRequest call with this (1-based) number is
     * held back until the test sets the value to 0 */
    size_t holdFinishRequest;

    /* A second CertificateGroup, served when not null. Its requests are
     * answered with issuedCertificate2 and issuedPrivateKey2. */
    UA_NodeId groupId2;
    UA_ByteString issuedCertificate2; /* not owned */
    UA_ByteString issuedPrivateKey2;  /* not owned */

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
    UA_NodeId requestGroups[MAX_REQUESTS];
    UA_NodeId lastRequestId;
    UA_NodeId lastCertificateType;
    UA_ByteString lastCsr;

    /* One FinishRequest call (1-based, 0 for none) is answered with its own
     * mode after a delay. The delay blocks the CertificateManager thread but
     * not cmMutex. */
    size_t delayedFinishCall;
    FinishMode delayedFinishMode;
    UA_UInt32 delayedFinishMs;

    /* The next StartSigningRequest is answered after this delay (one-shot).
     * The delay blocks the CertificateManager thread but not cmMutex. */
    UA_UInt32 delayedStartSigningMs;

    /* RequestId argument of each FinishRequest call */
    UA_NodeId finishRequestIds[MAX_FINISH_CALLS];

    /* The SecureChannel opened last and the number of opened SecureChannels */
    UA_UInt32 lastChannelId;
    size_t channelsOpened;

    /* Call counters */
    size_t findApplications, registerApplication, getCertificateGroups,
        getCertificateStatus, startSigningRequest, startNewKeyPairRequest,
        finishRequest, getCertificates, getTrustList, open, read, close;
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
setAssigned(UA_Boolean assigned) {
    lockCm();
    cm.assigned = assigned;
    unlockCm();
}

static void
setIssuedCertificate(UA_ByteString certificate) {
    lockCm();
    cm.issuedCertificate = certificate;
    unlockCm();
}

/* The certificate and private key FinishRequest returns for a
 * StartNewKeyPairRequest */
static void
setIssuedKeyPair(UA_ByteString certificate, UA_ByteString privateKey) {
    lockCm();
    cm.issuedCertificate = certificate;
    cm.issuedPrivateKey = privateKey;
    unlockCm();
}

/* Serve a second CertificateGroup next to the DefaultApplicationGroup. The
 * requester is a member of both. Its requests are answered with this
 * certificate and private key. */
static void
serveSecondGroup(UA_ByteString certificate, UA_ByteString privateKey) {
    lockCm();
    cm.groupId2 = UA_NODEID_NUMERIC(
        cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    cm.issuedCertificate2 = certificate;
    cm.issuedPrivateKey2 = privateKey;
    unlockCm();
}

static void
sleepMs(UA_UInt32 ms) {
#ifdef UA_ARCHITECTURE_WIN32
    Sleep(ms);
#else
    struct timespec ts = {(time_t)(ms / 1000), (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
#endif
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

/* Like MOCK_METHOD, but the method first waits for the one-shot delay in the
 * given field. cmMutex is not held while waiting, so the tests can still take
 * snapshots. Later requests on the same connection are answered after this
 * one. */
#define MOCK_METHOD_DELAYED(name, delayField)                                \
    static UA_StatusCode name##Locked(const UA_Variant *input,               \
                                      UA_Variant *output);                   \
    static UA_StatusCode name(                                               \
        UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,      \
        const UA_NodeId *methodId, void *methodContext,                      \
        const UA_NodeId *objectId, void *objectContext, size_t inputSize,    \
        const UA_Variant *input, size_t outputSize, UA_Variant *output) {    \
        lockCm();                                                            \
        UA_UInt32 delay = cm.delayField;                                     \
        cm.delayField = 0;                                                   \
        unlockCm();                                                          \
        if(delay > 0)                                                        \
            sleepMs(delay);                                                  \
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
    if(!UA_NodeId_isNull(&cm.groupId2) && UA_NodeId_equal(id, &cm.groupId2))
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

/* The group of a checked CertificateGroupId argument */
static UA_NodeId
certificateGroupOf(const UA_Variant *v) {
    const UA_NodeId *id = (const UA_NodeId*)v->data;
    if(!UA_NodeId_isNull(&cm.groupId2) && UA_NodeId_equal(id, &cm.groupId2))
        return cm.groupId2;
    return cm.groupId;
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
    /* The application is always a member of the second group */
    UA_NodeId groups[2];
    size_t groupsSize = 0;
    if(cm.assigned)
        groups[groupsSize++] = cm.groupId;
    if(!UA_NodeId_isNull(&cm.groupId2))
        groups[groupsSize++] = cm.groupId2;
    return UA_Variant_setArrayCopy(&output[0], groups, groupsSize,
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
    /* Always TRUE for a group the application does not belong to */
    UA_Boolean updateRequired = cm.updateRequired || !cm.assigned;
    return UA_Variant_setScalarCopy(&output[0], &updateRequired,
                                    &UA_TYPES[UA_TYPES_BOOLEAN]);
}

/* Must be called with cmMutex held */
static UA_NodeId
addRequest(UA_NodeId groupId) {
    if(cm.requestsSize >= MAX_REQUESTS)
        return UA_NODEID_NULL;
    UA_NodeId id = UA_NODEID_NUMERIC(cm.ns, GDSID_FIRST_REQUEST + cm.nextRequest++);
    cm.requestGroups[cm.requestsSize] = groupId;
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
        cm.requestsSize--;
        cm.requestIds[i] = cm.requestIds[cm.requestsSize];
        cm.requestGroups[i] = cm.requestGroups[cm.requestsSize];
        return true;
    }
    return false;
}

/* 7.9.3: StartSigningRequest(ApplicationId, CertificateGroupId,
 *                             CertificateTypeId, CertificateRequest)
 *        -> RequestId */
MOCK_METHOD_DELAYED(mockStartSigningRequest, delayedStartSigningMs) {
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
    UA_NodeId requestId = addRequest(certificateGroupOf(&input[1]));
    if(UA_NodeId_isNull(&requestId))
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    cm.lastCertificateType = *(const UA_NodeId*)input[2].data;
    cm.assigned = true; /* the application joins the group */
    return UA_Variant_setScalarCopy(&output[0], &requestId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

/* 7.9.4: StartNewKeyPairRequest(ApplicationId, CertificateGroupId,
 *                                CertificateTypeId, SubjectName, DomainNames,
 *                                PrivateKeyFormat, PrivateKeyPassword)
 *        -> RequestId */
MOCK_METHOD(mockStartNewKeyPairRequest) {
    cm.startNewKeyPairRequest++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[2], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NodeId requestId = addRequest(certificateGroupOf(&input[1]));
    if(UA_NodeId_isNull(&requestId))
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    cm.lastCertificateType = *(const UA_NodeId*)input[2].data;
    cm.assigned = true; /* the application joins the group */
    return UA_Variant_setScalarCopy(&output[0], &requestId,
                                    &UA_TYPES[UA_TYPES_NODEID]);
}

/* 7.9.5: FinishRequest(ApplicationId, RequestId)
 *        -> Certificate, PrivateKey, IssuerCertificates[]
 * Must be called with cmMutex held */
static UA_StatusCode
answerFinishRequest(const UA_Variant *input, UA_Variant *output,
                    FinishMode mode) {
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(!isScalar(&input[1], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_NodeId *requestId = (const UA_NodeId*)input[1].data;

    const UA_NodeId *groupId = NULL;
    for(size_t i = 0; i < cm.requestsSize; i++) {
        if(UA_NodeId_equal(&cm.requestIds[i], requestId))
            groupId = &cm.requestGroups[i];
    }
    if(!groupId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* The certificate issued for the group of the request */
    UA_ByteString certificate = cm.issuedCertificate;
    UA_ByteString privateKey = cm.issuedPrivateKey;
    if(!UA_NodeId_isNull(&cm.groupId2) &&
       UA_NodeId_equal(groupId, &cm.groupId2)) {
        certificate = cm.issuedCertificate2;
        privateKey = cm.issuedPrivateKey2;
    }

    switch(mode) {
    case FINISH_PENDING:
        return UA_STATUSCODE_BADNOTHINGTODO;
    case FINISH_REJECT:
        removeRequest(requestId);
        return UA_STATUSCODE_BADREQUESTNOTALLOWED;
    case FINISH_DENIED:
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    case FINISH_APPROVE_LOST:
    case FINISH_APPROVE:
    default:
        break;
    }

    /* Like most CertificateManagers, the request is gone once the
     * certificate was handed out (7.9.5) */
    if(cm.keepIssuedRequests) {
        for(size_t i = 0; i < cm.keyDeliveredSize; i++) {
            if(UA_NodeId_equal(&cm.keyDelivered[i], requestId))
                privateKey = UA_BYTESTRING_NULL;
        }
        if(privateKey.length > 0 && cm.keyDeliveredSize < MAX_REQUESTS)
            cm.keyDelivered[cm.keyDeliveredSize++] = *requestId;
    } else {
        removeRequest(requestId);
    }
    cm.updateRequired = false; /* the application is up to date from now on */
    cm.certificateIssued = true;

    /* The CertificateManager completed the request, but the response gets
     * lost. The response is not sent on a closing SecureChannel. The Method
     * runs with the server lock held, which is recursive.
     * onCertificateManagerChannel takes cmMutex only for an opened
     * SecureChannel. */
    if(mode == FINISH_APPROVE_LOST) {
        if(cm.finishMode == FINISH_APPROVE_LOST)
            cm.finishMode = FINISH_APPROVE;
        return UA_Server_closeSecureChannel(cm.server, cm.lastChannelId,
                                            UA_SHUTDOWNREASON_CLOSE);
    }

    res = UA_Variant_setScalarCopy(&output[0], &certificate,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_Variant_setScalarCopy(&output[1], &privateKey,
                                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_Variant_setArrayCopy(&output[2], NULL, 0,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    return res;
}

/* Records the call and answers it. A delayed call is answered after the delay
 * with its own mode. The answer to a held call is computed right away and sent
 * once the test releases it. The CertificateManager thread blocks in both
 * cases (but not cmMutex), so later requests on the same connection are
 * answered after this one. */
static UA_StatusCode
mockFinishRequest(UA_Server *s, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *methodId,
                  void *methodContext, const UA_NodeId *objectId,
                  void *objectContext, size_t inputSize,
                  const UA_Variant *input, size_t outputSize,
                  UA_Variant *output) {
    lockCm();
    size_t call = ++cm.finishRequest;
    if(call <= MAX_FINISH_CALLS &&
       isScalar(&input[1], &UA_TYPES[UA_TYPES_NODEID]))
        cm.finishRequestIds[call - 1] = *(const UA_NodeId*)input[1].data;
    FinishMode mode = cm.finishMode;
    UA_UInt32 delay = 0;
    if(call == cm.delayedFinishCall) {
        mode = cm.delayedFinishMode;
        delay = cm.delayedFinishMs;
    }
    unlockCm();

    if(delay > 0)
        sleepMs(delay);

    lockCm();
    UA_StatusCode res = answerFinishRequest(input, output, mode);
    while(cm.holdFinishRequest == call) {
        unlockCm();
        sleepMs(1);
        lockCm();
    }
    unlockCm();
    return res;
}

/* Record the SecureChannels opened by the requester. Runs in the
 * CertificateManager thread. */
static void
onCertificateManagerChannel(UA_Server *s, UA_ApplicationNotificationType type,
                            const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED)
        return;
    const UA_UInt32 *channelId = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "securechannel-id"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(!channelId)
        return;
    lockCm();
    cm.lastChannelId = *channelId;
    cm.channelsOpened++;
    unlockCm();
}

/* 7.9.8: GetCertificates(ApplicationId, CertificateGroupId)
 *        -> CertificateTypeIds[], Certificates[]
 * Only the certificate issued last is current. */
MOCK_METHOD(mockGetCertificates) {
    cm.getCertificates++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    size_t size = cm.certificateIssued ? 1 : 0;
    res = UA_Variant_setArrayCopy(&output[0], &cm.lastCertificateType, size,
                                  &UA_TYPES[UA_TYPES_NODEID]);
    res |= UA_Variant_setArrayCopy(&output[1], &cm.issuedCertificate, size,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    return res;
}

/* 7.9.9: GetTrustList(ApplicationId, CertificateGroupId) -> TrustListId.
 * Both groups share the served TrustList. */
MOCK_METHOD(mockGetTrustList) {
    cm.getTrustList++;
    UA_StatusCode res = checkApplicationId(&input[0]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = checkCertificateGroupId(&input[1]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* Only for a group the application belongs to */
    if(!cm.assigned)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
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
 * unit test helpers, the EventLoop keeps the real clock. With rsaMin, the
 * deprecated SecurityPolicy#Basic128Rsa15 adds the certificate type
 * RsaMinApplicationCertificateType. */
static UA_Server *
newServer(UA_UInt16 port, const UA_ByteString *certificate,
          const UA_ByteString *privateKey, const UA_ByteString *trusted,
          const char *applicationUri, UA_Boolean rsaMin) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_INFO);
    ck_assert_uint_eq(UA_ServerConfig_setDefaultWithSecurityPolicies(
                          &config, port, certificate, privateKey, trusted, 1,
                          NULL, 0, NULL, 0),
                      UA_STATUSCODE_GOOD);
    if(rsaMin) {
        ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyBasic128Rsa15(
                              &config, certificate, privateKey),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_ServerConfig_addEndpoint(
                              &config,
                              UA_STRING("http://opcfoundation.org/UA/"
                                        "SecurityPolicy#Basic128Rsa15"),
                              UA_MESSAGESECURITYMODE_SIGNANDENCRYPT),
                          UA_STATUSCODE_GOOD);
    }
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
    cm.server = newServer(GDS_PORT, &gdsCert, &gdsKey, &requesterCert, GDS_URI,
                          false);
    UA_Server_getConfig(cm.server)->secureChannelNotificationCallback =
        onCertificateManagerChannel;

    cm.ns = UA_Server_addNamespace(cm.server, GDS_NS_URI);
    cm.applicationId = UA_NODEID_NUMERIC(cm.ns, GDSID_APPLICATION);
    cm.groupId = UA_NODEID_NUMERIC(
        cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    cm.trustListId = UA_NODEID_NUMERIC(
        cm.ns, GDSID_DIRECTORY_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    cm.updateRequired = true;
    cm.assigned = true;
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

    UA_Argument in[7], out[3];

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
    in[3] = argument("SubjectName", string, UA_VALUERANK_SCALAR);
    in[4] = argument("DomainNames", string, UA_VALUERANK_ONE_DIMENSION);
    in[5] = argument("PrivateKeyFormat", string, UA_VALUERANK_SCALAR);
    in[6] = argument("PrivateKeyPassword", string, UA_VALUERANK_SCALAR);
    addDirectoryMethod(GDSID_DIRECTORY_STARTNEWKEYPAIRREQUEST, "StartNewKeyPairRequest",
                       mockStartNewKeyPairRequest, 7, in, 1, out);

    in[1] = argument("RequestId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("Certificate", byteString, UA_VALUERANK_SCALAR);
    out[1] = argument("PrivateKey", byteString, UA_VALUERANK_SCALAR);
    out[2] = argument("IssuerCertificates", byteString, UA_VALUERANK_ONE_DIMENSION);
    addDirectoryMethod(GDSID_DIRECTORY_FINISHREQUEST, "FinishRequest",
                       mockFinishRequest, 2, in, 3, out);

    in[1] = argument("CertificateGroupId", nodeId, UA_VALUERANK_SCALAR);
    out[0] = argument("CertificateTypeIds", nodeId, UA_VALUERANK_ONE_DIMENSION);
    out[1] = argument("Certificates", byteString, UA_VALUERANK_ONE_DIMENSION);
    addDirectoryMethod(GDSID_DIRECTORY_GETCERTIFICATES, "GetCertificates",
                       mockGetCertificates, 2, in, 2, out);

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
    /* Release a held FinishRequest of a failed test */
    lockCm();
    cm.holdFinishRequest = 0;
    unlockCm();
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
    UA_DateTime time; /* monotonic */
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
    n->time = UA_DateTime_nowMonotonic();
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

/* Time between the n-th and the (n+1)-th notification of the type */
static UA_DateTime
timeBetween(UA_GDSPullRequesterNotification type, size_t n) {
    return notifications[positionOf(type, n + 1)].time -
           notifications[positionOf(type, n)].time;
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

/* Iterate the requester for the given real time */
static void
iterateFor(UA_UInt32 ms) {
    UA_DateTime end =
        UA_DateTime_nowMonotonic() + (UA_DateTime)ms * UA_DATETIME_MSEC;
    while(UA_DateTime_nowMonotonic() < end)
        iterate();
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

/* Change the cycle timeout of the requester before the server is started. The
 * driver reads its params when it is started. */
static void
setCycleTimeout(UA_UInt32 cycleTimeout) {
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&pull->drv.params,
                                               UA_QUALIFIEDNAME(0, "cycle-timeout"),
                                               &cycleTimeout, &UA_TYPES[UA_TYPES_UINT32]),
                      UA_STATUSCODE_GOOD);
}

/* Change the cycle interval of the requester before the server is started */
static void
setCycleInterval(UA_UInt32 cycleInterval) {
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&pull->drv.params,
                                               UA_QUALIFIEDNAME(0, "cycle-interval"),
                                               &cycleInterval, &UA_TYPES[UA_TYPES_UINT32]),
                      UA_STATUSCODE_GOOD);
}

/* Let the CertificateManager create the key pair (StartNewKeyPairRequest).
 * Must be called before the server is started. */
static void
setCreatePrivateKey(UA_Boolean createPrivateKey) {
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&pull->drv.params,
                                               UA_QUALIFIEDNAME(0, "create-private-key"),
                                               &createPrivateKey,
                                               &UA_TYPES[UA_TYPES_BOOLEAN]),
                      UA_STATUSCODE_GOOD);
}

/* Map the local DefaultUserTokenGroup to the second group of the
 * CertificateManager as well. Must be called before the server is started. */
static void
mapSecondGroup(void) {
    UA_NodeId local[2] = {defaultApplicationGroup, defaultUserTokenGroup};
    UA_NodeId remote[2] = {cm.groupId, cm.groupId2};
    UA_Variant v;
    UA_Variant_setArray(&v, local, 2, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_KeyValueMap_set(&pull->drv.params,
                                         UA_QUALIFIEDNAME(0, "mappings-local-group-id"),
                                         &v),
                      UA_STATUSCODE_GOOD);
    UA_Variant_setArray(&v, remote, 2, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_KeyValueMap_set(&pull->drv.params,
                                         UA_QUALIFIEDNAME(0, "mappings-remote-group-id"),
                                         &v),
                      UA_STATUSCODE_GOOD);
}

static UA_SecurityPolicy *
findSecurityPolicy(const char *policyUri) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String uri = UA_STRING((char*)(uintptr_t)policyUri);
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &uri))
            return &config->securityPolicies[i];
    }
    ck_abort_msg("SecurityPolicy %s not found", policyUri);
    return NULL;
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
                       REQUESTER_URI, false);
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

/* Replace the requester server of the fixture before it is started. With
 * SecurityPolicy#Basic128Rsa15, the DefaultApplicationGroup has a second
 * certificate type, so one cycle starts two signing requests. */
static void
replaceRequesterServerWithRsaMin(void) {
    ck_assert_uint_eq(UA_Server_removeDriver(server, &pull->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pull->drv.free(&pull->drv), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
    server = newServer(REQUESTER_PORT, &requesterCert, &requesterKey, &gdsCert,
                       REQUESTER_URI, true);
    ck_assert_uint_eq(UA_Server_addNamespace(server, GDS_NS_URI), cm.ns);
    pull = newRequester(GDS_ENDPOINT, 0, NULL);
    ck_assert_uint_eq(UA_Server_addDriver(server, &pull->drv),
                      UA_STATUSCODE_GOOD);
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
    local.specifiedLists = UA_TRUSTLISTMASKS_ALL;
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

/* The SecurityPolicies of the local group, and the endpoints that use them,
 * present the certificate */
static void
assertGroupCertificateIs(const UA_NodeId *certificateGroupId,
                         const UA_ByteString *certificate) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t checked = 0;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &config->securityPolicies[i];
        if(!UA_NodeId_equal(&sp->certificateGroupId, certificateGroupId))
            continue;
        ck_assert_msg(UA_ByteString_equal(&sp->localCertificate, certificate),
                      "SecurityPolicy %.*s of CertificateGroup %u presents "
                      "another certificate", (int)sp->policyUri.length,
                      (const char*)sp->policyUri.data,
                      certificateGroupId->identifier.numeric);
        for(size_t j = 0; j < config->endpointsSize; j++) {
            UA_EndpointDescription *ed = &config->endpoints[j];
            if(UA_String_equal(&ed->securityPolicyUri, &sp->policyUri))
                ck_assert(UA_ByteString_equal(&ed->serverCertificate, certificate));
        }
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

/* 7.6 Table 21: FinishRequest returns a Bad result other than Bad_NothingToDo
 * that is not a rejection, here Bad_UserAccessDenied (7.9.5). The request is
 * dropped and a new request is sent in the next cycle. It must not be kept and
 * asked again forever. */
START_TEST(deniedRequestIsReplacedInNextCycle) {
    setFinishMode(FINISH_DENIED);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId firstRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_DROPPED, 1),
                         &firstRequest, &c.lastCertificateType);

    /* Still denied: the next cycle starts another request */
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();
    UA_NodeId secondRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startSigningRequest, 2);
    ck_assert_uint_eq(c.finishRequest, 2);
    ck_assert(!UA_NodeId_equal(&secondRequest, &firstRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 2);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_DROPPED, 2),
                         &secondRequest, &c.lastCertificateType);

    /* The new request of the third cycle is approved */
    setFinishMode(FINISH_APPROVE);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 3));
    c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 3);
    ck_assert(!UA_NodeId_equal(&c.lastRequestId, &secondRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 3);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &c.lastRequestId, &c.lastCertificateType);
} END_TEST

/* 7.6: A pending request supplied at creation is finished directly with
 * FinishRequest. No new signing request is started for it. */
START_TEST(restoredPendingRequestIsFinishedWithoutNewRequest) {
    lockCm();
    UA_NodeId pendingRequest = addRequest(cm.groupId);
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

static void
setScalarParam(const char *key, const void *p, const UA_DataType *type) {
    UA_QualifiedName name = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&pull->drv.params, name, p, type),
                      UA_STATUSCODE_GOOD);
}

/* The REQUEST_ADDED payload has scalar NodeIds. A request restored as scalars
 * instead of NodeId arrays is not taken as "no pending requests". The driver
 * does not start, so no second signing request is started for the pair. */
START_TEST(restoredPendingRequestAsScalarsIsRejected) {
    lockCm();
    UA_NodeId pendingRequest = addRequest(cm.groupId);
    unlockCm();
    ck_assert(!UA_NodeId_isNull(&pendingRequest));
    replaceRequester(GDS_ENDPOINT, 0, NULL);
    setScalarParam("pending-request-id", &pendingRequest,
                   &UA_TYPES[UA_TYPES_NODEID]);
    setScalarParam("pending-certificate-group-id", &defaultApplicationGroup,
                   &UA_TYPES[UA_TYPES_NODEID]);
    setScalarParam("pending-certificate-type-id", &rsaSha256CertificateType,
                   &UA_TYPES[UA_TYPES_NODEID]);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    serverStarted = true;
    iterateFor(2 * CYCLE_INTERVAL_MS);
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(pull->drv.start(&pull->drv),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_uint_eq(notificationsSize, 0);
} END_TEST

/* A parameter of another type than documented is rejected and does not fall
 * back to the default or to the current value */
START_TEST(wrongTypedParamsAreRejected) {
    startup();
    stopRequester();

    UA_Int32 int32 = CYCLE_INTERVAL_MS;
    UA_UInt16 uint16 = 2000;
    UA_UInt32 uint32 = 1;
    UA_String string = UA_STRING("app");
    struct {
        const char *key;
        const void *wrong;
        const UA_DataType *wrongType;
    } cases[] = {
        {"cycle-interval", &int32, &UA_TYPES[UA_TYPES_INT32]},
        {"cycle-timeout", &uint16, &UA_TYPES[UA_TYPES_UINT16]},
        {"create-private-key", &uint32, &UA_TYPES[UA_TYPES_UINT32]},
        {"application-id", &string, &UA_TYPES[UA_TYPES_STRING]},
        {"mappings-local-group-id", &defaultApplicationGroup,
         &UA_TYPES[UA_TYPES_NODEID]}
    };
    for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        UA_QualifiedName key =
            UA_QUALIFIEDNAME(0, (char*)(uintptr_t)cases[i].key);
        UA_Variant valid;
        UA_Variant_init(&valid);
        const UA_Variant *current = UA_KeyValueMap_get(&pull->drv.params, key);
        UA_Boolean present = (current != NULL);
        if(present)
            ck_assert_uint_eq(UA_Variant_copy(current, &valid),
                              UA_STATUSCODE_GOOD);

        setScalarParam(cases[i].key, cases[i].wrong, cases[i].wrongType);
        ck_assert_msg(pull->drv.start(&pull->drv) ==
                      UA_STATUSCODE_BADCONFIGURATIONERROR,
                      "%s of the wrong type is accepted", cases[i].key);
        ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);

        /* Restore the valid value */
        if(present) {
            ck_assert_uint_eq(UA_KeyValueMap_set(&pull->drv.params, key,
                                                 &valid),
                              UA_STATUSCODE_GOOD);
            UA_Variant_clear(&valid);
        } else {
            ck_assert_uint_eq(UA_KeyValueMap_remove(&pull->drv.params, key),
                              UA_STATUSCODE_GOOD);
        }
    }

    /* The restored params start the driver again */
    ck_assert_uint_eq(pull->drv.start(&pull->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* Without a reachable CertificateManager the cycle ends without any request
 * and the driver keeps running until the server shuts down. Depending on the
 * host, the connection attempt is refused or hangs until the cycle times
 * out. */
START_TEST(unreachableCertificateManager) {
    replaceRequester(UNUSED_ENDPOINT, 0, NULL);
    setCycleTimeout(2000);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_STARTED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_FINISHED), 1);
    ck_assert_uint_eq(notificationsSize, 2);
    ck_assert_uint_eq(snapshot().getCertificateStatus, 0);

    shutdownRequester();
} END_TEST

/* A cycle that does not finish in time is aborted while the connection is
 * still being opened. The next cycle starts as usual. */
START_TEST(cycleTimeoutAbortsCycle) {
    setCycleTimeout(1);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CYCLE_STARTED), 2);
    ck_assert_uint_eq(snapshot().getCertificateGroups, 0);

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

/* Notification callback that stops the driver as soon as a cycle starts */
static void
stopOnCycleStarted(UA_GDSPullRequester *requester, void *context,
                   UA_GDSPullRequesterNotification type,
                   const UA_KeyValueMap payload) {
    onNotification(requester, context, type, payload);
    if(type == UA_GDSPULL_CYCLE_STARTED)
        requester->drv.stop(&requester->drv);
}

/* The driver can be stopped from the notification callback when a cycle
 * starts. The cycle ends without contacting the CertificateManager, the
 * driver reaches STOPPED and no further cycle starts. */
START_TEST(stopFromCycleStartedNotification) {
    UA_GDSPull_setNotificationCallback(pull, stopOnCycleStarted, pull);
    startup();

    UA_DateTime end = deadline();
    while(pull->drv.state != UA_LIFECYCLESTATE_STOPPED &&
          UA_DateTime_nowMonotonic() < end)
        iterate();
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);

    /* Give the EventLoop time to process the closed connection and a possible
     * next cycle */
    end = UA_DateTime_nowMonotonic() +
        2 * CYCLE_INTERVAL_MS * UA_DATETIME_MSEC;
    while(UA_DateTime_nowMonotonic() < end)
        iterate();

    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(notificationsSize, 2);
    ck_assert_uint_eq(positionOf(UA_GDSPULL_CYCLE_STARTED, 1), 0);
    ck_assert_uint_eq(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), 1);
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.getCertificateGroups, 0);
    ck_assert_uint_eq(c.getCertificateStatus, 0);

    shutdownRequester();
} END_TEST

/* 7.6: A FinishRequest result belongs to the RequestId it was called for. The
 * driver is stopped while FinishRequest is in flight and the application tries
 * to start it again right away. The driver must not reload its pending
 * requests while the old cycle is still being torn down. The answer for the
 * second request would otherwise be applied to the first reloaded one. */
START_TEST(startWhileStoppingKeepsRequestBinding) {
    lockCm();
    UA_NodeId pendingRequests[2];
    pendingRequests[0] = addRequest(cm.groupId);
    pendingRequests[1] = addRequest(cm.groupId);
    cm.holdFinishRequest = 2;
    unlockCm();
    replaceRequester(GDS_ENDPOINT, 2, pendingRequests);

    /* The first request is finished. The answer for the second one is held
     * back by the CertificateManager. */
    startup();
    UA_DateTime end = deadline();
    while(snapshot().finishRequest < 2 && UA_DateTime_nowMonotonic() < end)
        iterate();
    ck_assert_uint_eq(snapshot().finishRequest, 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);

    /* The pending-* params still hold both requests */
    pull->drv.stop(&pull->drv);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPING);
    UA_StatusCode startRes = pull->drv.start(&pull->drv);

    lockCm();
    cm.holdFinishRequest = 0;
    unlockCm();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    /* Each request is reported once, with its own RequestId */
    ck_assert_uint_eq(c.finishRequest, 2);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &pendingRequests[0], &rsaSha256CertificateType);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 2),
                         &pendingRequests[1], &rsaSha256CertificateType);

    /* The start was refused. The driver stopped and can be started again. */
    ck_assert_uint_eq(startRes, UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(pull->drv.start(&pull->drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_STARTED, 2));
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

/* 7.9.10, 7.9.3: The application does not belong to the CertificateGroup
 * yet, so GetCertificateGroups returns an empty list. GetCertificateStatus
 * reports UpdateRequired for the group and StartSigningRequest joins it. The
 * certificate is issued and the TrustList of the joined group is read in the
 * same cycle. */
START_TEST(unassignedGroupIsJoinedBySigningRequest) {
    setAssigned(false);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    ck_assert_uint_eq(c.getCertificateGroups, 1);
    ck_assert_uint_gt(c.getCertificateStatus, 0);
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.getTrustList, 1);
    ck_assert(c.assigned);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_UPDATED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_TRUSTLIST_FAILED), 0);
    ck_assert(UA_NodeId_equal(
        payloadNodeId(positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1),
                      "certificate-group-id"),
        &defaultApplicationGroup));
    assertLocalCertificateIs(&requesterCert);
    assertLocalTrustListEquals(&c.trustList);

    /* The next cycle finds the group assigned and does not request again */
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();
    ck_assert_uint_eq(c.getCertificateGroups, 2);
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.getTrustList, 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* A secure client of the requester (Basic256Sha256, SignAndEncrypt). The
 * state callback records whether its SecureChannel left the OPEN state after
 * the Session was activated, and the status it left with. Only the first
 * close is recorded. */
static UA_Boolean clientSessionActivated;
static UA_Boolean clientChannelClosed;
static UA_StatusCode clientCloseStatus;

static void
secureClientStateCallback(UA_Client *client,
                          UA_SecureChannelState channelState,
                          UA_SessionState sessionState,
                          UA_StatusCode connectStatus) {
    if(sessionState == UA_SESSIONSTATE_ACTIVATED)
        clientSessionActivated = true;
    else if(clientSessionActivated && !clientChannelClosed &&
            channelState != UA_SECURECHANNELSTATE_OPEN) {
        clientChannelClosed = true;
        clientCloseStatus = connectStatus;
    }
}

/* A secureChannelLifeTime of 0 keeps the default of the client */
static UA_Client *
connectSecureClientWithPolicy(const char *securityPolicyUri,
                              UA_ByteString certificate,
                              UA_ByteString privateKey,
                              const char *applicationUri,
                              UA_UInt32 secureChannelLifeTime) {
    clientSessionActivated = false;
    clientChannelClosed = false;
    clientCloseStatus = UA_STATUSCODE_GOOD;
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_StatusCode res = UA_ClientConfig_setDefaultEncryption(
        cc, certificate, privateKey, NULL, 0, NULL, 0);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    cc->securityPolicyUri = UA_STRING_ALLOC(securityPolicyUri);
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC(applicationUri);
    if(secureChannelLifeTime > 0)
        cc->secureChannelLifeTime = secureChannelLifeTime;
    cc->stateCallback = secureClientStateCallback;

    char url[32];
    snprintf(url, sizeof(url), "opc.tcp://localhost:%u", REQUESTER_PORT);
    ck_assert_uint_eq(UA_Client_connectAsync(client, url), UA_STATUSCODE_GOOD);

    UA_SessionState sessionState = UA_SESSIONSTATE_CLOSED;
    UA_StatusCode connectStatus = UA_STATUSCODE_GOOD;
    UA_DateTime end = deadline();
    while(sessionState != UA_SESSIONSTATE_ACTIVATED &&
          connectStatus == UA_STATUSCODE_GOOD &&
          UA_DateTime_nowMonotonic() < end) {
        UA_Client_run_iterate(client, 0);
        UA_Server_run_iterate(server, false);
        UA_Client_getState(client, NULL, &sessionState, &connectStatus);
    }
    ck_assert_uint_eq(connectStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessionState, UA_SESSIONSTATE_ACTIVATED);
    return client;
}

static UA_Client *
connectSecureClient(UA_ByteString certificate, UA_ByteString privateKey,
                    const char *applicationUri,
                    UA_UInt32 secureChannelLifeTime) {
    return connectSecureClientWithPolicy(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256",
        certificate, privateKey, applicationUri, secureChannelLifeTime);
}

/* Iterate the requester and the client for the given time */
static void
runWithClient(UA_Client *client, UA_UInt32 ms) {
    UA_DateTime end = UA_DateTime_nowMonotonic() + ms * UA_DATETIME_MSEC;
    while(UA_DateTime_nowMonotonic() < end) {
        UA_Client_run_iterate(client, 0);
        UA_Server_run_iterate(server, false);
    }
}

/* Iterate the requester and the client until the n-th notification of the
 * given type was seen */
static UA_Boolean
runWithClientUntil(UA_Client *client, UA_GDSPullRequesterNotification type,
                   size_t n) {
    UA_DateTime end = deadline();
    while(countNotifications(type) < n && UA_DateTime_nowMonotonic() < end) {
        UA_Client_run_iterate(client, 0);
        UA_Server_run_iterate(server, false);
    }
    return countNotifications(type) >= n;
}

/* 7.6, 7.10.9: A downloaded TrustList is applied to the existing
 * SecureChannels. The certificate of a connected client is removed from the
 * TrustList of the CertificateManager. The requester closes its
 * SecureChannel. */
START_TEST(trustListUpdateClosesUntrustedChannels) {
    setUpdateRequired(false);
    UA_ByteString served[3] = {gdsCert, requesterCert, foreignCert};
    setServedTrustList(served, 3);
    startup();
    ck_assert(runUntil(UA_GDSPULL_TRUSTLIST_UPDATED, 1));

    /* The client is trusted by the downloaded TrustList */
    UA_Client *client = connectSecureClient(foreignCert, foreignKey,
                                            "urn:open62541.test.foreign", 0);
    runWithClient(client, 200);
    ck_assert(!clientChannelClosed);

    /* The client is no longer trusted */
    setServedTrustList(served, 2);
    ck_assert(runUntil(UA_GDSPULL_TRUSTLIST_UPDATED, 2));
    ck_assert_uint_eq(payloadStatusCode(positionOf(UA_GDSPULL_TRUSTLIST_UPDATED, 2)),
                      UA_STATUSCODE_GOOD);
    MockCertificateManager c = snapshot();
    assertLocalTrustListEquals(&c.trustList);

    runWithClient(client, 500);
    ck_assert_msg(clientChannelClosed, "The SecureChannel of the client whose "
                  "certificate was removed from the TrustList is still open");

    UA_Client_disconnect(client);
    UA_Client_delete(client);
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

static size_t
countFinishRequests(const MockCertificateManager *c,
                    const UA_NodeId *requestId) {
    size_t count = 0;
    size_t calls = (c->finishRequest < MAX_FINISH_CALLS) ? c->finishRequest
                                                          : MAX_FINISH_CALLS;
    for(size_t i = 0; i < calls; i++)
        count += UA_NodeId_equal(&c->finishRequestIds[i], requestId);
    return count;
}

/* 7.6, 7.9.5: The connection to the CertificateManager is lost while the
 * driver waits to repeat FinishRequest after Bad_NothingToDo. The client
 * reconnects within the cycle. The repetition is still sent only once, and
 * each FinishRequest result is applied to the request it was sent for.
 *
 * Request A is answered with Bad_NothingToDo, then the SecureChannel is
 * closed by the CertificateManager. The next FinishRequest for A is approved
 * after a delay. A FinishRequest for A that is sent twice is answered for
 * the erased request with Bad_InvalidArgument. Request B stays pending. */
START_TEST(reconnectDuringFinishRetryKeepsRequestBinding) {
    replaceRequesterServerWithRsaMin();
    lockCm();
    cm.finishMode = FINISH_PENDING;
    cm.delayedFinishCall = 2;
    cm.delayedFinishMode = FINISH_APPROVE;
    cm.delayedFinishMs = 1500;
    unlockCm();

    startup();

    /* The first FinishRequest (for A) is answered with Bad_NothingToDo. Give
     * the driver time to process the answer, then drop the connection. */
    UA_DateTime end = deadline();
    while(snapshot().finishRequest < 1 && UA_DateTime_nowMonotonic() < end)
        iterate();
    ck_assert_uint_eq(snapshot().finishRequest, 1);
    iterateFor(200);
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.finishRequest, 1);
    size_t channelsBefore = c.channelsOpened;
    ck_assert_uint_eq(UA_Server_closeSecureChannel(cm.server, c.lastChannelId,
                                                   UA_SHUTDOWNREASON_CLOSE),
                      UA_STATUSCODE_GOOD);

    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    c = snapshot();

    /* The client reconnected within the cycle */
    ck_assert_uint_gt(c.channelsOpened, channelsBefore);

    /* Two requests were started: A for RsaSha256 and B for RsaMin */
    ck_assert_uint_eq(c.startSigningRequest, 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    UA_NodeId requestA, requestB;
    ck_assert_uint_eq(
        UA_NodeId_copy(payloadNodeId(positionOf(UA_GDSPULL_REQUEST_ADDED, 1),
                                     "request-id"), &requestA),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_NodeId_copy(payloadNodeId(positionOf(UA_GDSPULL_REQUEST_ADDED, 2),
                                     "request-id"), &requestB),
        UA_STATUSCODE_GOOD);

    /* A is finished. B was never answered with a final result and is still
     * pending at the CertificateManager and in the requester. */
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1), &requestA,
                         &rsaSha256CertificateType);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert(UA_NodeId_equal(&c.requestIds[0], &requestB));
    assertLocalCertificateIs(&requesterCert);

    /* A was asked once before and once after the reconnect. B was asked at
     * most 1 + 2 times (Bad_NothingToDo is repeated twice). */
    ck_assert_uint_eq(countFinishRequests(&c, &requestA), 2);
    ck_assert_uint_ge(countFinishRequests(&c, &requestB), 1);
    ck_assert_uint_le(countFinishRequests(&c, &requestB), 3);

    /* B is approved in the next cycle without a new signing request */
    setFinishMode(FINISH_APPROVE);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();
    ck_assert_uint_eq(c.startSigningRequest, 2);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 2);
    ck_assert(UA_NodeId_equal(
        payloadNodeId(positionOf(UA_GDSPULL_REQUEST_FINISHED, 2), "request-id"),
        &requestB));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);

    UA_NodeId_clear(&requestA);
    UA_NodeId_clear(&requestB);
} END_TEST

/* 6.3: The second cycle starts with a random delay of 1% to 10% of the cycle
 * period, so that applications started at the same time do not contact the
 * CertificateManager at the same time in every cycle. The cycles after it keep
 * the period. */
#define RANDOM_DELAY_INTERVAL_MS 2000
#define RANDOM_DELAY_TOLERANCE_MS 5

START_TEST(secondCycleStartsWithRandomDelay) {
    const UA_DateTime period = RANDOM_DELAY_INTERVAL_MS * UA_DATETIME_MSEC;
    const UA_DateTime tolerance = RANDOM_DELAY_TOLERANCE_MS * UA_DATETIME_MSEC;
    setCycleInterval(RANDOM_DELAY_INTERVAL_MS);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_STARTED, 3));
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);

    UA_DateTime second = timeBetween(UA_GDSPULL_CYCLE_STARTED, 1);
    UA_DateTime third = timeBetween(UA_GDSPULL_CYCLE_STARTED, 2);
    printf("Cycle 1 -> 2: %lld ms, cycle 2 -> 3: %lld ms\n",
           (long long)(second / UA_DATETIME_MSEC),
           (long long)(third / UA_DATETIME_MSEC));

    /* The second cycle is delayed by at least 1% of the period ... */
    ck_assert_int_ge(second, period + period / 100 - tolerance);
    /* ... and by at most 10% (plus the scheduling latency) */
    ck_assert_int_le(second, period + period / 10 + 20 * tolerance);
    /* The third cycle follows the second one after one period */
    ck_assert_int_ge(third, period - period / 100);
    ck_assert_int_le(third, period + period / 100);

    shutdownRequester();
} END_TEST

/* 7.6, 7.9.3: The cycle times out while StartSigningRequest is processed. The
 * response still arrives before the connection is closed. The RequestId the
 * CertificateManager created is kept and finished in the next cycle instead
 * of starting a second request. */
START_TEST(startSigningResponseAfterCycleTimeoutIsKept) {
    setFinishMode(FINISH_PENDING);
    setCycleTimeout(1000);
    lockCm();
    cm.delayedStartSigningMs = 1500; /* answered after the cycle timeout */
    unlockCm();

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId firstRequest = c.lastRequestId;
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_ADDED, 1),
                         &firstRequest, &c.lastCertificateType);
    ck_assert_uint_lt(positionOf(UA_GDSPULL_REQUEST_ADDED, 1),
                      positionOf(UA_GDSPULL_CYCLE_FINISHED, 1));

    /* The next cycle asks for the result of the kept request */
    size_t finishRequestsBefore = c.finishRequest;
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert_uint_gt(c.finishRequest, finishRequestsBefore);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FAILED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.6, 7.9.5: The cycle times out while FinishRequest is processed. The
 * CertificateManager hands the certificate over and forgets the request. The
 * response still arrives before the connection is closed, so the certificate
 * must be installed from it. */
START_TEST(finishResponseAfterCycleTimeoutIsApplied) {
    lockCm();
    UA_NodeId pendingRequest = addRequest(cm.groupId);
    cm.delayedFinishCall = 1; /* answered after the cycle timeout */
    cm.delayedFinishMode = FINISH_APPROVE;
    cm.delayedFinishMs = 1500;
    unlockCm();
    ck_assert(!UA_NodeId_isNull(&pendingRequest));
    replaceRequester(GDS_ENDPOINT, 1, &pendingRequest);
    setCycleTimeout(1000);

    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &pendingRequest, &rsaSha256CertificateType);
    size_t installed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    ck_assert(UA_NodeId_equal(payloadNodeId(installed, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert(UA_NodeId_equal(payloadNodeId(installed, "certificate-type-id"),
                              &rsaSha256CertificateType));
    ck_assert_uint_eq(payloadStatusCode(installed), UA_STATUSCODE_GOOD);
    ck_assert_uint_lt(installed, positionOf(UA_GDSPULL_CYCLE_FINISHED, 1));
    assertLocalCertificateIs(&requesterCert);

    /* The finished request is not asked for again */
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

#ifdef CAN_RENEW_CERTIFICATE
/* 7.9.5: The CertificateManager completed the request and erased it, but the
 * FinishRequest response was lost with the SecureChannel. The RequestId is
 * kept, and the next FinishRequest for it returns Bad_InvalidArgument. The
 * request was made with StartSigningRequest, so the certificate is retrieved
 * with GetCertificates (7.9.8) and installed. GetCertificateStatus reports no
 * update from now on, the certificate would be lost otherwise. */
START_TEST(lostFinishResponseIsRecoveredWithGetCertificates) {
    setIssuedCertificate(renewedCert);
    setFinishMode(FINISH_APPROVE_LOST);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId lostRequest = c.lastRequestId;

    /* The request is kept for the next cycle */
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    assertLocalCertificateIs(&requesterCert);

    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    /* FinishRequest is asked once more, then the certificate is retrieved
     * instead of starting a new request */
    ck_assert_uint_eq(c.finishRequest, 2);
    ck_assert_uint_eq(c.getCertificates, 1);
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 0);
    size_t cycle = positionOf(UA_GDSPULL_CYCLE_FINISHED, 1);
    size_t dropped = positionOf(UA_GDSPULL_REQUEST_DROPPED, 1);
    size_t installed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    ck_assert_uint_lt(cycle, dropped);
    ck_assert_uint_lt(dropped, installed);
    ck_assert_uint_lt(installed, positionOf(UA_GDSPULL_CYCLE_FINISHED, 2));
    assertRequestPayload(dropped, &lostRequest, &rsaSha256CertificateType);
    ck_assert(UA_NodeId_equal(payloadNodeId(installed, "certificate-group-id"),
                              &defaultApplicationGroup));
    ck_assert(UA_NodeId_equal(payloadNodeId(installed, "certificate-type-id"),
                              &rsaSha256CertificateType));
    ck_assert_uint_eq(payloadStatusCode(installed), UA_STATUSCODE_GOOD);
    assertLocalCertificateIs(&renewedCert);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST
#endif

/* 7.9.5: Same as above, but the request was made with
 * StartNewKeyPairRequest. The private key was handed out with the lost
 * response, so the process is restarted with a new StartNewKeyPairRequest
 * although GetCertificateStatus reports no update. */
START_TEST(lostNewKeyPairResponseRestartsRequest) {
    setCreatePrivateKey(true);
    lockCm();
    cm.issuedCertificate = foreignCert;
    cm.issuedPrivateKey = foreignKey;
    cm.finishMode = FINISH_APPROVE_LOST;
    unlockCm();
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId lostRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startNewKeyPairRequest, 1);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);

    /* The new request is not approved yet */
    setFinishMode(FINISH_PENDING);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    ck_assert(!c.updateRequired);
    ck_assert_uint_eq(c.startNewKeyPairRequest, 2);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert(!UA_NodeId_equal(&c.lastRequestId, &lostRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_DROPPED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    size_t dropped = positionOf(UA_GDSPULL_REQUEST_DROPPED, 1);
    size_t added = positionOf(UA_GDSPULL_REQUEST_ADDED, 2);
    ck_assert_uint_lt(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), dropped);
    ck_assert_uint_lt(dropped, added);
    ck_assert_uint_lt(added, positionOf(UA_GDSPULL_CYCLE_FINISHED, 2));
    assertRequestPayload(dropped, &lostRequest, &rsaSha256CertificateType);
    assertRequestPayload(added, &c.lastRequestId, &rsaSha256CertificateType);
    assertLocalCertificateIs(&requesterCert);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.9.5: A CertificateManager that answers a repeated FinishRequest hands
 * out the private key of StartNewKeyPairRequest only once. After a lost
 * response, the repeated answer carries the certificate without the key. It
 * cannot be installed, and the process is restarted with a new
 * StartNewKeyPairRequest. */
START_TEST(repeatedNewKeyPairResponseWithoutKeyRestartsRequest) {
    setCreatePrivateKey(true);
    lockCm();
    cm.issuedCertificate = foreignCert;
    cm.issuedPrivateKey = foreignKey;
    cm.keepIssuedRequests = true;
    cm.finishMode = FINISH_APPROVE_LOST;
    unlockCm();
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId lostRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startNewKeyPairRequest, 1);
    ck_assert_uint_eq(c.finishRequest, 1);
    ck_assert_uint_eq(c.keyDeliveredSize, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);

    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    /* The repeated answer without the key fails, the new request brings
     * certificate and key */
    ck_assert(!c.updateRequired);
    ck_assert_uint_eq(c.startNewKeyPairRequest, 2);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.keyDeliveredSize, 2);
    ck_assert(!UA_NodeId_equal(&c.lastRequestId, &lostRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    size_t failed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED, 1);
    size_t added = positionOf(UA_GDSPULL_REQUEST_ADDED, 2);
    size_t installed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    ck_assert_uint_lt(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), failed);
    ck_assert_uint_lt(failed, added);
    ck_assert_uint_lt(added, installed);
    ck_assert_uint_lt(installed, positionOf(UA_GDSPULL_CYCLE_FINISHED, 2));
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_FINISHED, 1),
                         &lostRequest, &rsaSha256CertificateType);
    assertRequestPayload(added, &c.lastRequestId, &rsaSha256CertificateType);
    ck_assert_uint_eq(payloadStatusCode(installed), UA_STATUSCODE_GOOD);
    assertLocalCertificateIs(&foreignCert);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.6: The issued certificate could not be installed, but the
 * CertificateManager considers it the current one and no longer reports
 * UpdateRequired. The application still needs a certificate, so a new request
 * is sent in the next cycle. */
START_TEST(failedInstallationIsReRequestedInNextCycle) {
    setIssuedCertificate(foreignCert);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    UA_NodeId failedRequest = c.lastRequestId;

    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert(!c.updateRequired);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 0);
    assertLocalCertificateIs(&requesterCert);

    /* The CertificateManager issues a matching certificate for the new
     * request */
    setIssuedCertificate(requesterCert);
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 2));
    c = snapshot();

    ck_assert_uint_eq(c.startSigningRequest, 2);
    ck_assert_uint_eq(c.finishRequest, 2);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert(!UA_NodeId_equal(&c.lastRequestId, &failedRequest));
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);
    size_t added = positionOf(UA_GDSPULL_REQUEST_ADDED, 2);
    ck_assert_uint_lt(positionOf(UA_GDSPULL_CYCLE_FINISHED, 1), added);
    assertRequestPayload(added, &c.lastRequestId, &rsaSha256CertificateType);
    size_t installed = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    ck_assert_uint_lt(added, installed);
    ck_assert_uint_eq(payloadStatusCode(installed), UA_STATUSCODE_GOOD);
    assertLocalCertificateIs(&requesterCert);

    /* Installed now, so the third cycle follows GetCertificateStatus again */
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 3));
    ck_assert_uint_eq(snapshot().startSigningRequest, 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 2);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* The SecurityToken of the client is renewed after 75% of its lifetime */
#define CHANNEL_CLIENT_LIFETIME_MS 2000

/* 7.6, 7.10.9: A new certificate and private key from StartNewKeyPairRequest
 * are installed while a client is connected. The SecureChannel was opened for
 * the old certificate. The requester closes it in an orderly way right after
 * the installation, so that the client has to reconnect to the new
 * certificate. It must not stay open until its next token renewal, which
 * fails because the renewal is checked against the new certificate and
 * key. */
START_TEST(installedCertificateClosesExistingChannels) {
    setCreatePrivateKey(true);
    setIssuedKeyPair(newKeyPairCert, newKeyPairKey);
    setFinishMode(FINISH_PENDING);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();
    ck_assert_uint_eq(c.startNewKeyPairRequest, 1);
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.requestsSize, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    assertLocalCertificateIs(&requesterCert);

    /* Connect to the old certificate, then let the next cycle install the
     * new one. The client authenticates with gdsCert, which the requester
     * trusts. */
    UA_Client *client = connectSecureClient(gdsCert, gdsKey, GDS_URI,
                                            CHANNEL_CLIENT_LIFETIME_MS);
    setFinishMode(FINISH_APPROVE);
    ck_assert(runWithClientUntil(client, UA_GDSPULL_CERTIFICATE_INSTALLED, 1));
    UA_DateTime installed = UA_DateTime_nowMonotonic();
    ck_assert_uint_eq(payloadStatusCode(positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1)),
                      UA_STATUSCODE_GOOD);
    assertLocalCertificateIs(&newKeyPairCert);

    /* Iterate until the SecureChannel is closed. The time covers at least one
     * token renewal of the client. */
    UA_DateTime end = installed +
        (UA_DateTime)(3 * CHANNEL_CLIENT_LIFETIME_MS / 2) * UA_DATETIME_MSEC;
    while(!clientChannelClosed && UA_DateTime_nowMonotonic() < end) {
        UA_Client_run_iterate(client, 0);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_msg(clientChannelClosed, "The SecureChannel opened for the old "
                  "certificate is still open");
    UA_Double closedAfterMs =
        (UA_Double)(UA_DateTime_nowMonotonic() - installed) / UA_DATETIME_MSEC;
    ck_assert_msg(clientCloseStatus != UA_STATUSCODE_BADCERTIFICATEINVALID &&
                  clientCloseStatus != UA_STATUSCODE_BADSECURITYCHECKSFAILED,
                  "The SecureChannel opened for the old certificate failed with "
                  "%s %.0f ms after the installation instead of being closed",
                  UA_StatusCode_name(clientCloseStatus), closedAfterMs);
    /* Closed right away, not at the next token renewal */
    ck_assert_msg(closedAfterMs < CHANNEL_CLIENT_LIFETIME_MS / 4,
                  "The SecureChannel was closed %.0f ms after the installation "
                  "(%s)", closedAfterMs, UA_StatusCode_name(clientCloseStatus));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* 7.9.4, 7.9.5: A certificate is requested for a (CertificateGroup,
 * CertificateType) pair. The local DefaultApplicationGroup and
 * DefaultUserTokenGroup both have RsaSha256 SecurityPolicies, so two requests
 * are made in one cycle. Each issued certificate replaces the certificate of
 * its own group only. */
START_TEST(certificateIsInstalledIntoRequestedGroupOnly) {
    findSecurityPolicy(AES128_POLICY_URI)->certificateGroupId =
        defaultUserTokenGroup;
    setIssuedKeyPair(newKeyPairCert, newKeyPairKey);
    serveSecondGroup(userTokenGroupCert, userTokenGroupKey);
    setCreatePrivateKey(true);
    mapSecondGroup();
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    /* One request per group */
    ck_assert_uint_eq(c.startSigningRequest, 0);
    ck_assert_uint_eq(c.startNewKeyPairRequest, 2);
    ck_assert_uint_eq(c.finishRequest, 2);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_FINISHED), 2);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 2);
    size_t first = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1);
    size_t second = positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 2);
    ck_assert_uint_eq(payloadStatusCode(first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(payloadStatusCode(second), UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_equal(payloadNodeId(first, "certificate-group-id"),
                               payloadNodeId(second, "certificate-group-id")));

    assertGroupCertificateIs(&defaultApplicationGroup, &newKeyPairCert);
    assertGroupCertificateIs(&defaultUserTokenGroup, &userTokenGroupCert);
} END_TEST

/* Remove the endpoints of a SecurityPolicy before the server is started. The
 * SecurityPolicy stays configured. */
static void
removeEndpointsOf(const char *policyUri) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String uri = UA_STRING((char*)(uintptr_t)policyUri);
    size_t removed = 0;
    for(size_t i = 0; i < config->endpointsSize;) {
        if(!UA_String_equal(&config->endpoints[i].securityPolicyUri, &uri)) {
            i++;
            continue;
        }
        UA_EndpointDescription_clear(&config->endpoints[i]);
        config->endpointsSize--;
        memmove(&config->endpoints[i], &config->endpoints[i + 1],
                (config->endpointsSize - i) * sizeof(UA_EndpointDescription));
        removed++;
    }
    ck_assert_uint_gt(removed, 0);
}

/* 7.6: GetCertificateStatus is called "for each needed CertificateType". A
 * (CertificateGroup, CertificateType) pair whose SecurityPolicies have no
 * endpoint cannot be presented to a client, and a certificate for it cannot
 * be installed. It is not requested, neither in the first cycle nor again in
 * later cycles. */
START_TEST(pairWithoutEndpointIsNotRequested) {
    findSecurityPolicy(AES128_POLICY_URI)->certificateGroupId =
        defaultUserTokenGroup;
    removeEndpointsOf(AES128_POLICY_URI);
    serveSecondGroup(requesterCert, UA_BYTESTRING_NULL);
    mapSecondGroup();
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    MockCertificateManager c = snapshot();

    /* Only the DefaultApplicationGroup is requested */
    ck_assert_uint_eq(c.startSigningRequest, 1);
    ck_assert_uint_eq(c.requestsSize, 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    assertRequestPayload(positionOf(UA_GDSPULL_REQUEST_ADDED, 1),
                         &c.lastRequestId, &rsaSha256CertificateType);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 0);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLED), 1);

    /* No request is started for the pair in later cycles either */
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 3));
    ck_assert_uint_eq(snapshot().startSigningRequest, 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_REQUEST_ADDED), 1);
    ck_assert_uint_eq(countNotifications(UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED), 0);
    ck_assert_uint_eq(pull->drv.state, UA_LIFECYCLESTATE_STARTED);
} END_TEST

/* 7.10.9: The SecureChannels of a CertificateGroup are closed when its
 * certificate is replaced. A SecureChannel of a SecurityPolicy in another
 * CertificateGroup with the same CertificateType keeps its certificate, so
 * it stays open and its token renewals succeed. Only the
 * DefaultApplicationGroup is pull-managed. */
START_TEST(installedCertificateKeepsChannelsOfOtherGroups) {
    findSecurityPolicy(AES128_POLICY_URI)->certificateGroupId =
        defaultUserTokenGroup;
    setCreatePrivateKey(true);
    setIssuedKeyPair(newKeyPairCert, newKeyPairKey);
    setFinishMode(FINISH_PENDING);
    startup();
    ck_assert(runUntil(UA_GDSPULL_CYCLE_FINISHED, 1));
    ck_assert_uint_eq(snapshot().requestsSize, 1);

    /* Connect through the DefaultUserTokenGroup, then let the next cycle
     * install the certificate of the DefaultApplicationGroup */
    UA_Client *client =
        connectSecureClientWithPolicy(AES128_POLICY_URI, gdsCert, gdsKey,
                                      GDS_URI, CHANNEL_CLIENT_LIFETIME_MS);
    setFinishMode(FINISH_APPROVE);
    ck_assert(runWithClientUntil(client, UA_GDSPULL_CERTIFICATE_INSTALLED, 1));
    ck_assert_uint_eq(payloadStatusCode(positionOf(UA_GDSPULL_CERTIFICATE_INSTALLED, 1)),
                      UA_STATUSCODE_GOOD);
    assertGroupCertificateIs(&defaultApplicationGroup, &newKeyPairCert);
    ck_assert(UA_ByteString_equal(
        &findSecurityPolicy(AES128_POLICY_URI)->localCertificate,
        &requesterCert));

    /* The time covers at least one token renewal of the client */
    runWithClient(client, 3 * CHANNEL_CLIENT_LIFETIME_MS / 2);
    ck_assert_msg(!clientChannelClosed, "The SecureChannel of the "
                  "DefaultUserTokenGroup was closed (%s)",
                  UA_StatusCode_name(clientCloseStatus));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
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

#ifdef CAN_RENEW_CERTIFICATE
/* Issue a second self-signed certificate for the key of a certificate. The
 * subject and the SubjectAltName are taken over, the serial number and the
 * validity differ. */
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
static UA_StatusCode
renewCertificate(const UA_ByteString *certificate,
                 const UA_ByteString *privateKey, UA_ByteString *renewed) {
    const unsigned char *p = certificate->data;
    X509 *crt = d2i_X509(NULL, &p, (long)certificate->length);
    p = privateKey->data;
    EVP_PKEY *key = d2i_AutoPrivateKey(NULL, &p, (long)privateKey->length);
    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    int len = 0;
    if(!crt || !key ||
       ASN1_INTEGER_set(X509_get_serialNumber(crt), 4711) != 1 ||
       !X509_gmtime_adj(X509_getm_notBefore(crt), -60) ||
       X509_sign(crt, key, EVP_sha256()) <= 0)
        goto cleanup;
    len = i2d_X509(crt, NULL);
    if(len <= 0)
        goto cleanup;
    res = UA_ByteString_allocBuffer(renewed, (size_t)len);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    unsigned char *out = renewed->data;
    if(i2d_X509(crt, &out) != len) {
        UA_ByteString_clear(renewed);
        res = UA_STATUSCODE_BADINTERNALERROR;
    }
cleanup:
    EVP_PKEY_free(key);
    X509_free(crt);
    return res;
}
#else
#define MAX_SAN_ENTRIES 8
static UA_StatusCode
renewCertificate(const UA_ByteString *certificate,
                 const UA_ByteString *privateKey, UA_ByteString *renewed) {
    mbedtls_x509_crt crt;
    mbedtls_pk_context key;
    mbedtls_x509write_cert out;
    mbedtls_x509_san_list san[MAX_SAN_ENTRIES];
    size_t sanSize = 0;
    mbedtls_x509_crt_init(&crt);
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&out);

    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    char name[512];
    unsigned char serial[2] = {0x12, 0x67};
    unsigned char buf[4096];
    int len = 0;
    if(mbedtls_x509_crt_parse_der(&crt, certificate->data,
                                  certificate->length) != 0 ||
       UA_mbedTLS_compat_parsePrivateKey(&key, privateKey->data,
                                         privateKey->length, NULL, 0) != 0)
        goto cleanup;

    mbedtls_x509write_crt_set_version(&out, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&out, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&out, &key);
    mbedtls_x509write_crt_set_issuer_key(&out, &key);
    if(mbedtls_x509_dn_gets(name, sizeof(name), &crt.subject) <= 0 ||
       mbedtls_x509write_crt_set_subject_name(&out, name) != 0 ||
       mbedtls_x509write_crt_set_issuer_name(&out, name) != 0 ||
       mbedtls_x509write_crt_set_serial_raw(&out, serial, sizeof(serial)) != 0 ||
       mbedtls_x509write_crt_set_validity(&out, "20240101000000",
                                          "20991231235959") != 0)
        goto cleanup;

    for(const mbedtls_x509_sequence *cur = &crt.subject_alt_names;
        cur && cur->buf.p; cur = cur->next) {
        if(sanSize == MAX_SAN_ENTRIES ||
           mbedtls_x509_parse_subject_alt_name(&cur->buf, &san[sanSize].node) != 0)
            goto cleanup;
        san[sanSize].next = NULL;
        if(sanSize > 0)
            san[sanSize - 1].next = &san[sanSize];
        sanSize++;
    }
    if(sanSize > 0 &&
       mbedtls_x509write_crt_set_subject_alternative_name(&out, san) != 0)
        goto cleanup;

    len = UA_mbedTLS_compat_writeCertificateDer(&out, buf, sizeof(buf));
    if(len <= 0)
        goto cleanup;
    res = UA_ByteString_allocBuffer(renewed, (size_t)len);
    if(res == UA_STATUSCODE_GOOD)
        memcpy(renewed->data, buf + sizeof(buf) - (size_t)len, (size_t)len);

cleanup:
    for(size_t i = 0; i < sanSize; i++)
        mbedtls_x509_free_subject_alt_name(&san[i].node);
    mbedtls_x509write_crt_free(&out);
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&crt);
    return res;
}
#endif
#endif /* CAN_RENEW_CERTIFICATE */

int
main(void) {
    createCertificate("CN=GDS Pull Requester", "URI:" REQUESTER_URI,
                      &requesterKey, &requesterCert);
    createCertificate("CN=Mock CertificateManager", "URI:" GDS_URI,
                      &gdsKey, &gdsCert);
    createCertificate("CN=Foreign Application", "URI:urn:open62541.test.foreign",
                      &foreignKey, &foreignCert);
    createCertificate("CN=GDS Pull Requester", "URI:" REQUESTER_URI,
                      &newKeyPairKey, &newKeyPairCert);
    createCertificate("CN=GDS Pull Requester UserToken", "URI:" REQUESTER_URI,
                      &userTokenGroupKey, &userTokenGroupCert);
#ifdef CAN_RENEW_CERTIFICATE
    if(renewCertificate(&requesterCert, &requesterKey, &renewedCert) !=
       UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Renewing the requester certificate failed\n");
        exit(EXIT_FAILURE);
    }
#endif

    Suite *suite = suite_create("GDS Pull Requester lifecycle");
    TCase *tc = tcase_create("lifecycle");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, issueCertificateAndUpdateTrustList);
    tcase_add_test(tc, noUpdateRequiredSkipsSigningRequest);
    tcase_add_test(tc, pendingRequestIsFinishedInLaterCycle);
    tcase_add_test(tc, rejectedRequestIsReplacedInNextCycle);
    tcase_add_test(tc, deniedRequestIsReplacedInNextCycle);
    tcase_add_test(tc, restoredPendingRequestIsFinishedWithoutNewRequest);
    tcase_add_test(tc, restoredPendingRequestAsScalarsIsRejected);
    tcase_add_test(tc, wrongTypedParamsAreRejected);
    tcase_add_test(tc, unreachableCertificateManager);
    tcase_add_test(tc, cycleTimeoutAbortsCycle);
    tcase_add_test(tc, stopDuringCycleAndReplaceRequester);
    tcase_add_test(tc, stopFromCycleStartedNotification);
    tcase_add_test(tc, startWhileStoppingKeepsRequestBinding);
    tcase_add_test(tc, unchangedTrustListIsNotDownloadedAgain);
    tcase_add_test(tc, changedTrustListIsDownloadedAgain);
    tcase_add_test(tc, trustListUpdateClosesUntrustedChannels);
    tcase_add_test(tc, certificateNotMatchingKeyIsNotInstalled);
    tcase_add_test(tc, reconnectDuringFinishRetryKeepsRequestBinding);
    tcase_add_test(tc, secondCycleStartsWithRandomDelay);
    tcase_add_test(tc, unassignedGroupIsJoinedBySigningRequest);
    tcase_add_test(tc, startSigningResponseAfterCycleTimeoutIsKept);
    tcase_add_test(tc, finishResponseAfterCycleTimeoutIsApplied);
#ifdef CAN_RENEW_CERTIFICATE
    tcase_add_test(tc, lostFinishResponseIsRecoveredWithGetCertificates);
#endif
    tcase_add_test(tc, lostNewKeyPairResponseRestartsRequest);
    tcase_add_test(tc, repeatedNewKeyPairResponseWithoutKeyRestartsRequest);
    tcase_add_test(tc, failedInstallationIsReRequestedInNextCycle);
    tcase_add_test(tc, installedCertificateClosesExistingChannels);
    tcase_add_test(tc, certificateIsInstalledIntoRequestedGroupOnly);
    tcase_add_test(tc, pairWithoutEndpointIsNotRequested);
    tcase_add_test(tc, installedCertificateKeepsChannelsOfOtherGroups);
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
    UA_ByteString_clear(&newKeyPairCert);
    UA_ByteString_clear(&newKeyPairKey);
    UA_ByteString_clear(&userTokenGroupCert);
    UA_ByteString_clear(&userTokenGroupKey);
#ifdef CAN_RENEW_CERTIFICATE
    UA_ByteString_clear(&renewedCert);
#endif
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
