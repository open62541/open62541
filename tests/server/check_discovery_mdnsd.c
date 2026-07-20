/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef UA_ARCHITECTURE_WIN32
#include <time.h>
#else
#include <windows.h>
#endif

#include <open62541/client_config_default.h>
#include <open62541/client.h>
#include <open62541/driver/mdns.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_discovery.h"
#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)
#include "src_generated/mdnsd/mdnsd.h"
#include "src_generated/mdnsd/sdtxt.h"
#endif
#include "test_helpers.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

#if defined(UA_ENABLE_DISCOVERY) && \
    (defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD) || \
     defined(UA_ENABLE_DISCOVERY_MULTICAST_AVAHI))

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_AVAHI)
# define MDNS_DRIVER_CONSTRUCTOR UA_MdnsDriver_Avahi
# define MDNS_DRIVER_NAME "discovery-mdns-avahi"
# define MDNS_DRIVER_SUITE_NAME "Discovery Avahi"
#else
# define MDNS_DRIVER_CONSTRUCTOR UA_MdnsDriver_Mdnsd
# define MDNS_DRIVER_NAME "discovery-mdns"
# define MDNS_DRIVER_SUITE_NAME "Discovery mDNSd"
#endif

static UA_MdnsDriver *
newDriver(UA_Boolean listen, UA_Boolean announce, UA_Boolean queryPresence,
          UA_Boolean queryDetails, UA_UInt32 queryInterval,
          UA_UInt32 announceTTL) {
    UA_KeyValuePair params[6];
    params[0].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[0].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[1].key = UA_QUALIFIEDNAME(0, "announce");
    UA_Variant_setScalar(&params[1].value, &announce, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "query-presence");
    UA_Variant_setScalar(&params[2].value, &queryPresence,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[3].key = UA_QUALIFIEDNAME(0, "query-details");
    UA_Variant_setScalar(&params[3].value, &queryDetails,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[4].key = UA_QUALIFIEDNAME(0, "query-interval");
    UA_Variant_setScalar(&params[4].value, &queryInterval,
                         &UA_TYPES[UA_TYPES_UINT32]);
    params[5].key = UA_QUALIFIEDNAME(0, "announce-ttl");
    UA_Variant_setScalar(&params[5].value, &announceTTL,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap paramsMap = {6, params};
    UA_MdnsDriver *mdns = MDNS_DRIVER_CONSTRUCTOR(paramsMap);
    ck_assert_ptr_ne(mdns, NULL);
    return mdns;
}

START_TEST(DriverConstructionCopiesConfig) {
    UA_MdnsDriver *mdns = newDriver(true, true, true, true, 30, 4242);
    UA_String expectedName = UA_STRING(MDNS_DRIVER_NAME);
    ck_assert(UA_String_equal(&mdns->drv.name, &expectedName));
    ck_assert(mdns->addARecord != NULL);
    ck_assert(mdns->removeARecord != NULL);
    ck_assert(mdns->addAAAARecord != NULL);
    ck_assert(mdns->removeAAAARecord != NULL);

    const UA_Boolean *queryPresence = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&mdns->drv.params,
                                 UA_QUALIFIEDNAME(0, "query-presence"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *queryDetails = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&mdns->drv.params,
                                 UA_QUALIFIEDNAME(0, "query-details"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_UInt32 *queryInterval = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&mdns->drv.params,
                                 UA_QUALIFIEDNAME(0, "query-interval"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    const UA_UInt32 *announceTTL = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&mdns->drv.params,
                                 UA_QUALIFIEDNAME(0, "announce-ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(queryPresence, NULL);
    ck_assert_ptr_ne(queryDetails, NULL);
    ck_assert_ptr_ne(queryInterval, NULL);
    ck_assert_ptr_ne(announceTTL, NULL);
    ck_assert(*queryPresence);
    ck_assert(*queryDetails);
    ck_assert_uint_eq(*queryInterval, 30);
    ck_assert_uint_eq(*announceTTL, 4242);

    ck_assert_uint_eq(mdns->drv.free(&mdns->drv), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(DriverStartsAndStops) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);
    UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

    UA_MdnsDriver *mdns = newDriver(false, false, false, false, 0, 0);
    ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
}
END_TEST

START_TEST(QueryConfigWithListenFalseStarts) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);
    UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

    UA_MdnsDriver *mdns = newDriver(false, false, true, true, 30, 0);
    ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
}
END_TEST

START_TEST(LocalRegisterDeregisterPath) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);
    UA_Server_getConfig(localServer)->serversOnNetworkEnabled = true;

    UA_MdnsDriver *mdns = newDriver(false, false, false, false, 0, 120);
    ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

    UA_ServerOnNetwork son;
    UA_ServerOnNetwork_init(&son);
    son.serverName = UA_STRING("mdns-driver-local-test");
    son.discoveryUrl = UA_STRING("opc.tcp://localhost:4840");
    son.serverCapabilitiesSize = 1;
    UA_String caps = UA_STRING("NA");
    son.serverCapabilities = &caps;

    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(localServer, &son,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(localServer, false);
    ck_assert_uint_eq(UA_Server_deregisterServerOnNetwork(localServer,
                                                          son.serverName),
                      UA_STATUSCODE_GOOD);

    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
}
END_TEST

static void
addDriverInterfaceTests(Suite *s) {
    TCase *tc_driver = tcase_create("Driver interface");
    tcase_add_test(tc_driver, DriverConstructionCopiesConfig);
    tcase_add_test(tc_driver, DriverStartsAndStops);
    tcase_add_test(tc_driver, QueryConfigWithListenFalseStarts);
    tcase_add_test(tc_driver, LocalRegisterDeregisterPath);
    suite_add_tcase(s, tc_driver);
}

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)

#define TEST_UDP_CAPTURED_MESSAGES 64
#define TEST_MDNS_RECV_CONNECTION_ID 100

static UA_Server *server;
static UA_ConnectionManager *testUdpCm;
static UA_ConnectionManager *testUdpCmLds;
static UA_ConnectionManager *testUdpCmRegister;

static UA_Server *serverLds;
static UA_Server *serverRegister;
static UA_Boolean *runningLds;
static UA_Boolean *runningRegister;
static THREAD_HANDLE serverThreadLds;
static THREAD_HANDLE serverThreadRegister;

typedef struct {
    size_t sentMessages;
    size_t sentNonEmptyMessages;
    UA_ByteString lastMessage;
    UA_ByteString recentMessages[TEST_UDP_CAPTURED_MESSAGES];
    size_t recentMessageSequence[TEST_UDP_CAPTURED_MESSAGES];
    size_t recentMessageCursor;
} TestUdpIntercept;

typedef struct {
    const char *serverName;
    const char *path;
    const char *caps;
    UA_UInt16 port;
    UA_Boolean requireSrv;
    UA_UInt32 ttl;
    UA_Boolean checkTtl;
} MdnsMessageExpectation;

typedef struct {
    UA_UInt32 serverOnNetworkCalls;
    UA_UInt32 serverOnNetworkAnnounceCalls;
    UA_UInt32 serverOnNetworkRemoveCalls;
    UA_Boolean lastIsServerAnnounce;
    UA_Boolean lastIsTxtReceived;
    char lastServerName[128];
    char lastDiscoveryUrl[256];
    char lastCapabilities[128];
} DiscoveryIntegrationCounters;

static DiscoveryIntegrationCounters discoveryCounters;
static TestUdpIntercept *testUdpIntercept;
static TestUdpIntercept *testUdpInterceptLds;
static TestUdpIntercept *testUdpInterceptRegister;
static size_t globalInterceptedMdnsMessages;

/* Callbacks that run on the server threads must not ck_assert: with CK_NOFORK
 * a failing assert there writes to libcheck's messaging file after it was
 * closed. They record failures here instead; a checked fixture asserts the
 * counter on the main thread after every test. */
static size_t threadCallbackFailures;

static void
recordThreadCallbackFailure(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    threadCallbackFailures++;
}

static void
checkThreadCallbackFailures(void) {
    size_t failures = threadCallbackFailures;
    threadCallbackFailures = 0;
    ck_assert_uint_eq(failures, 0);
}

static void
serverDiscoveryNotificationCallback(UA_Server *server,
                                    UA_ApplicationNotificationType type,
                                    const UA_KeyValueMap payload);

static void
iterateDiscoveryServers(size_t iterations);

static UA_Boolean
stringHasPrefix(const char *str, const char *prefix);

static void
resetDiscoveryCounters(void) {
    memset(&discoveryCounters, 0, sizeof(discoveryCounters));
}

static void
addMdnsDriverWithQueries(UA_Server *s, UA_Boolean listen, UA_Boolean announce,
                         UA_Boolean queryPresence, UA_Boolean queryDetails,
                         UA_UInt32 queryInterval) {
    UA_KeyValuePair params[5];
    params[0].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[0].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[1].key = UA_QUALIFIEDNAME(0, "announce");
    UA_Variant_setScalar(&params[1].value, &announce, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "query-presence");
    UA_Variant_setScalar(&params[2].value, &queryPresence,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[3].key = UA_QUALIFIEDNAME(0, "query-details");
    UA_Variant_setScalar(&params[3].value, &queryDetails,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[4].key = UA_QUALIFIEDNAME(0, "query-interval");
    UA_Variant_setScalar(&params[4].value, &queryInterval,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap paramsMap = {5, params};
    UA_MdnsDriver *mdns = UA_MdnsDriver_Mdnsd(paramsMap);
    ck_assert_ptr_ne(mdns, NULL);
    ck_assert_uint_eq(UA_Server_addDriver(s, &mdns->drv), UA_STATUSCODE_GOOD);
}

static void
addMdnsDriver(UA_Server *s, UA_Boolean listen, UA_Boolean announce) {
    addMdnsDriverWithQueries(s, listen, announce, false, false, 0);
}

static void
registerServerOnNetwork(UA_Server *s, const char *serverName,
                        const char *discoveryUrl,
                        const char **capabilities, size_t capabilitiesSize) {
    UA_ServerOnNetwork son;
    UA_ServerOnNetwork_init(&son);
    son.serverName = UA_String_fromChars(serverName);
    son.discoveryUrl = UA_String_fromChars(discoveryUrl);
    son.serverCapabilitiesSize = capabilitiesSize;
    if(capabilitiesSize > 0) {
        son.serverCapabilities =
            (UA_String*)UA_Array_new(capabilitiesSize, &UA_TYPES[UA_TYPES_STRING]);
        ck_assert_ptr_ne(son.serverCapabilities, NULL);
        for(size_t i = 0; i < capabilitiesSize; i++)
            son.serverCapabilities[i] = UA_String_fromChars(capabilities[i]);
    }

    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(s, &son, UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);
    UA_ServerOnNetwork_clear(&son);
}

static void
deregisterServerOnNetwork(UA_Server *s, const char *serverName,
                          const char *discoveryUrl) {
    UA_String sonServerName = UA_String_fromChars(serverName);
    UA_String sonDiscoveryUrl = UA_String_fromChars(discoveryUrl);
    ck_assert_uint_eq(UA_Server_deregisterServerOnNetwork(s, sonServerName),
                      UA_STATUSCODE_GOOD);
    UA_String_clear(&sonServerName);
    UA_String_clear(&sonDiscoveryUrl);
}

static UA_UInt32
getServerOnNetworkRecordIdCounter(UA_Server *s) {
    return s->serversOnNetworkRecordCounter;
}

static UA_Driver *
getMdnsDriver(UA_Server *s) {
    UA_String mdnsName = UA_STRING("discovery-mdns");
    for(UA_Driver *drv = s->drivers; drv; drv = drv->next) {
        if(UA_String_equal(&drv->name, &mdnsName))
            return drv;
    }
    return NULL;
}

static void
copyUAStringToCString(const UA_String *src, char *dst, size_t dstSize) {
    if(dstSize == 0)
        return;

    dst[0] = '\0';
    if(!src || !src->data || src->length == 0)
        return;

    size_t copyLen = src->length;
    if(copyLen >= dstSize)
        copyLen = dstSize - 1;
    memcpy(dst, src->data, copyLen);
    dst[copyLen] = '\0';
}

static void
joinCapabilities(const UA_ServerOnNetwork *serverOnNetwork,
                 char *dst, size_t dstSize) {
    if(dstSize == 0)
        return;

    dst[0] = '\0';
    if(!serverOnNetwork)
        return;

    size_t offset = 0;
    for(size_t i = 0; i < serverOnNetwork->serverCapabilitiesSize; i++) {
        if(i > 0) {
            if(offset + 1 >= dstSize)
                break;
            dst[offset++] = ',';
        }

        size_t copyLen = serverOnNetwork->serverCapabilities[i].length;
        if(offset + copyLen >= dstSize)
            copyLen = dstSize - offset - 1;
        memcpy(dst + offset, serverOnNetwork->serverCapabilities[i].data, copyLen);
        offset += copyLen;
        if(offset + 1 >= dstSize)
            break;
    }
    dst[offset] = '\0';
}

static UA_Boolean
serverOnNetworkHasTxtData(const UA_ServerOnNetwork *serverOnNetwork) {
    if(!serverOnNetwork)
        return false;

    if(serverOnNetwork->serverCapabilitiesSize > 0)
        return true;

    const UA_String *url = &serverOnNetwork->discoveryUrl;
    if(!url->data || url->length == 0)
        return false;

    const char *urlData = (const char*)url->data;
    size_t pathStart = 0;
    if(stringHasPrefix(urlData, "opc.tcp://")) {
        pathStart = strlen("opc.tcp://");
    } else if(stringHasPrefix(urlData, "opc.wss://")) {
        pathStart = strlen("opc.wss://");
    }

    for(size_t i = pathStart; i < url->length; i++) {
        if(url->data[i] == '/')
            return true;
    }

    return false;
}

/* Runs on the server threads -- record failures instead of ck_assert
 * (see threadCallbackFailures) */
static UA_StatusCode
interceptingSend(UA_ConnectionManager *cm, uintptr_t connectionId,
                 const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)connectionId;
    (void)params;
    TestUdpIntercept *intercept =
        (TestUdpIntercept*)TestConnectionManager_getContext(cm);
    if(!intercept) {
        recordThreadCallbackFailure("interceptingSend: "
                                    "intercept context missing");
        UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    intercept->sentMessages++;
    if(buf->length > 0) {
        size_t slot = intercept->recentMessageCursor % TEST_UDP_CAPTURED_MESSAGES;
        intercept->sentNonEmptyMessages++;
        globalInterceptedMdnsMessages++;
        UA_ByteString_clear(&intercept->lastMessage);
        UA_ByteString_clear(&intercept->recentMessages[slot]);
        UA_StatusCode res = UA_ByteString_copy(buf, &intercept->lastMessage);
        res |= UA_ByteString_copy(buf, &intercept->recentMessages[slot]);
        if(res != UA_STATUSCODE_GOOD) {
            recordThreadCallbackFailure("interceptingSend: "
                                        "copying the message failed");
            UA_ByteString_clear(buf);
            return res;
        }
        intercept->recentMessageSequence[slot] = globalInterceptedMdnsMessages;
        intercept->recentMessageCursor++;
    }

    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static const TestConnectionManager_CallbackOverloads interceptingUdpOverloads = {
    NULL, interceptingSend, NULL
};

static void
clearTestUdpIntercept(TestUdpIntercept **intercept) {
    if(!*intercept)
        return;

    UA_ByteString_clear(&(*intercept)->lastMessage);
    for(size_t i = 0; i < TEST_UDP_CAPTURED_MESSAGES; i++)
        UA_ByteString_clear(&(*intercept)->recentMessages[i]);
    UA_free(*intercept);
    *intercept = NULL;
}

static size_t
getInterceptedMdnsSendCountFor(TestUdpIntercept *const *intercepts,
                               size_t interceptsSize) {
    size_t total = 0;
    for(size_t i = 0; i < interceptsSize; i++) {
        if(intercepts[i])
            total += intercepts[i]->sentNonEmptyMessages;
    }

    return total;
}

static size_t
getMaxInterceptedMdnsSequenceFor(TestUdpIntercept *const *intercepts,
                                 size_t interceptsSize) {
    size_t maxSeq = 0;
    for(size_t i = 0; i < interceptsSize; i++) {
        if(!intercepts[i])
            continue;
        for(size_t j = 0; j < TEST_UDP_CAPTURED_MESSAGES; j++)
            if(intercepts[i]->recentMessageSequence[j] > maxSeq)
                maxSeq = intercepts[i]->recentMessageSequence[j];
    }
    return maxSeq;
}

static size_t
getInterceptedMdnsSendCount(void) {
    TestUdpIntercept *intercepts[] = {testUdpInterceptLds, testUdpInterceptRegister};
    return getInterceptedMdnsSendCountFor(intercepts, 2);
}

static size_t
countServersOnNetwork(UA_Server *s) {
    return s->serversOnNetworkSize;
}

static size_t
countServersOnNetworkByName(UA_Server *s, const char *serverName) {
    size_t count = 0;
    for(size_t i = 0; i < s->serversOnNetworkSize; i++) {
        UA_ServerOnNetwork *son = &s->serversOnNetwork[i];
        if(!son->serverName.data || son->serverName.length != strlen(serverName))
            continue;
        if(memcmp(son->serverName.data, serverName, son->serverName.length) == 0)
            count++;
    }
    return count;
}

static void
updateMdnsForDiscoveryUrl(UA_Server *s, const char *serverName,
                          const char *discoveryUrl, UA_Boolean online) {
    UA_ServerConfig *config = UA_Server_getConfig(s);
    UA_ServerNotificationCallback callback = config->discoveryNotificationCallback;
    config->discoveryNotificationCallback = NULL;

    if(online) {
        const char *capabilities[] = {"NA"};
        registerServerOnNetwork(s, serverName, discoveryUrl, capabilities, 1);
    } else {
        deregisterServerOnNetwork(s, serverName, discoveryUrl);
    }

    config->discoveryNotificationCallback = callback;
}

static UA_Boolean
stringHasPrefix(const char *str, const char *prefix) {
    return str && prefix && strncmp(str, prefix, strlen(prefix)) == 0;
}

static UA_Boolean
stringHasSuffix(const char *str, const char *suffix) {
    if(!str || !suffix)
        return false;

    size_t strLen = strlen(str);
    size_t suffixLen = strlen(suffix);
    if(strLen < suffixLen)
        return false;

    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

static void
createInjectedServiceInstanceName(char *out, size_t outSize,
                                  const char *serverName,
                                  const char *hostname) {
    int written = snprintf(out, outSize, "%s-%s._opcua-tcp._tcp.local.",
                           serverName, hostname);
    ck_assert_int_gt(written, 0);
    ck_assert_int_lt(written, (int)outSize);
}

static void
createInjectedServerOnNetworkName(char *out, size_t outSize,
                                  const char *serverName,
                                  const char *hostname) {
    int written = snprintf(out, outSize, "%s-%s", serverName, hostname);
    ck_assert_int_gt(written, 0);
    ck_assert_int_lt(written, (int)outSize);
}

static void
createInjectedTargetName(char *out, size_t outSize, const char *hostname) {
    int written = snprintf(out, outSize, "%s.local.", hostname);
    ck_assert_int_gt(written, 0);
    ck_assert_int_lt(written, (int)outSize);
}

static UA_Boolean
isExpectedServiceInstanceName(const char *name, const MdnsMessageExpectation *expectation) {
    if(!stringHasPrefix(name, expectation->serverName))
        return false;
    char separator = name[strlen(expectation->serverName)];
    return (separator == '.' || separator == '-') &&
           stringHasSuffix(name, "._opcua-tcp._tcp.local.");
}

static const char *
findServiceInstanceNameInSection(const struct resource *records, size_t recordsSize,
                                 const MdnsMessageExpectation *expectation) {
    /* The mDNS announcement from mdnsd places the PTR record (which
     * references the service instance name) in the Authority section and
     * the matching SRV/TXT records in the Answer section. So we look in
     * all sections for any record that carries the instance name, not
     * just TXT/SRV types. */
    for(size_t i = 0; i < recordsSize; i++) {
        if(!isExpectedServiceInstanceName(records[i].name, expectation))
            continue;
        return records[i].name;
    }

    return NULL;
}

static const char *
findMatchingRecordNameInSection(const struct resource *records, size_t recordsSize,
                                const MdnsMessageExpectation *expectation) {
    for(size_t i = 0; i < recordsSize; i++) {
        if(!isExpectedServiceInstanceName(records[i].name, expectation))
            continue;
        return records[i].name;
    }

    return NULL;
}

static const char *
findServiceInstanceName(const struct message *message,
                        const MdnsMessageExpectation *expectation) {
    const char *instanceName =
        findServiceInstanceNameInSection(message->an, message->ancount, expectation);
    if(instanceName)
        return instanceName;

    instanceName =
        findServiceInstanceNameInSection(message->ns, message->nscount, expectation);
    if(instanceName)
        return instanceName;

    return findServiceInstanceNameInSection(message->ar, message->arcount, expectation);
}

static const char *
findMatchingRecordName(const struct message *message,
                       const MdnsMessageExpectation *expectation) {
    const char *instanceName =
        findMatchingRecordNameInSection(message->an, message->ancount, expectation);
    if(instanceName)
        return instanceName;

    instanceName =
        findMatchingRecordNameInSection(message->ns, message->nscount, expectation);
    if(instanceName)
        return instanceName;

    return findMatchingRecordNameInSection(message->ar, message->arcount, expectation);
}

static const struct question *
findNamedQuestionInSection(const struct question *questions, size_t questionsSize,
                           const char *name, unsigned short type) {
    for(size_t i = 0; i < questionsSize; i++) {
        if(questions[i].type == type && strcmp(questions[i].name, name) == 0)
            return &questions[i];
    }

    return NULL;
}

static const struct resource *
findNamedRecordInSection(const struct resource *records, size_t recordsSize,
                         const char *name, unsigned short type) {
    for(size_t i = 0; i < recordsSize; i++) {
        if(records[i].type == type && strcmp(records[i].name, name) == 0)
            return &records[i];
    }

    return NULL;
}

static const struct question *
findNamedQuestion(const struct message *message, const char *name,
                  unsigned short type) {
    const struct question *question =
        findNamedQuestionInSection(message->qd, message->qdcount, name, type);
    if(question)
        return question;

    return NULL;
}

static const struct resource *
findNamedRecord(const struct message *message, const char *name,
                unsigned short type) {
    const struct resource *rr =
        findNamedRecordInSection(message->an, message->ancount, name, type);
    if(rr)
        return rr;

    rr = findNamedRecordInSection(message->ns, message->nscount, name, type);
    if(rr)
        return rr;

    return findNamedRecordInSection(message->ar, message->arcount, name, type);
}

static const struct resource *
findAnyNamedRecordInSection(const struct resource *records, size_t recordsSize,
                            const char *name) {
    for(size_t i = 0; i < recordsSize; i++) {
        if(strcmp(records[i].name, name) == 0)
            return &records[i];
    }

    return NULL;
}

static const struct resource *
findAnyNamedRecord(const struct message *message, const char *name) {
    const struct resource *rr =
        findAnyNamedRecordInSection(message->an, message->ancount, name);
    if(rr)
        return rr;

    rr = findAnyNamedRecordInSection(message->ns, message->nscount, name);
    if(rr)
        return rr;

    return findAnyNamedRecordInSection(message->ar, message->arcount, name);
}

static UA_Boolean
txtRecordMatchesExpectation(const struct resource *txtRecord,
                            const MdnsMessageExpectation *expectation) {
    xht_t *txt = txt2sd(txtRecord->rdata, (int)txtRecord->rdlength);
    if(!txt)
        return false;

    char *path = (char*)xht_get(txt, "path");
    char *caps = (char*)xht_get(txt, "caps");
    UA_Boolean match =
        path && caps &&
        strcmp(path, expectation->path) == 0 &&
        strcmp(caps, expectation->caps) == 0;

    xht_free(txt);
    return match;
}

static UA_Boolean
mdnsPacketMatchesExpectation(const UA_ByteString *packet,
                             const MdnsMessageExpectation *expectation) {
    if(!packet || packet->length == 0 || packet->length >= MAX_PACKET_LEN)
        return false;

    unsigned char packetBuf[MAX_PACKET_LEN];
    memset(packetBuf, 0, sizeof(packetBuf));
    memcpy(packetBuf, packet->data, packet->length);

    struct message message;
    memset(&message, 0, sizeof(message));
    if(message_parse(&message, packetBuf) != 0)
        return false;
    UA_Boolean expectGoodbye = expectation->checkTtl && expectation->ttl == 0;
    const char *instanceName = findServiceInstanceName(&message, expectation);
    if(!instanceName && expectGoodbye)
        instanceName = findMatchingRecordName(&message, expectation);
    if(!instanceName && expectGoodbye) {
        /* Goodbye packet: the only answer is a PTR record (name=service
         * type, rdata=service instance name) with TTL=0. The instance
         * name is in the RDATA, not the record name. */
        for(int di = 0; di < message.ancount; di++) {
            if(message.an[di].type == QTYPE_PTR && message.an[di].ttl == 0 &&
               message.an[di].known.ptr.name &&
               isExpectedServiceInstanceName(message.an[di].known.ptr.name, expectation)) {
                instanceName = message.an[di].known.ptr.name;
                break;
            }
        }
    }
    if(!instanceName)
        return false;
    if(!expectGoodbye && (message.header.qr != 1 || message.qdcount != 0))
        return false;

    const struct question *txtQuestion =
        findNamedQuestion(&message, instanceName, QTYPE_TXT);
    const struct question *srvQuestion =
        findNamedQuestion(&message, instanceName, QTYPE_SRV);
    const struct resource *txtRecord = expectGoodbye ?
        findNamedRecord(&message, instanceName, QTYPE_TXT) :
        findNamedRecordInSection(message.an, message.ancount,
                                 instanceName, QTYPE_TXT);
    const struct resource *srvRecord = NULL;
    if(expectation->requireSrv)
        srvRecord = expectGoodbye ?
            findNamedRecord(&message, instanceName, QTYPE_SRV) :
            findNamedRecordInSection(message.an, message.ancount,
                                     instanceName, QTYPE_SRV);
    if(!txtRecord) {
        if(expectGoodbye) {
            /* Goodbye packet: accept a PTR record whose RDATA points to
             * the expected instance name and whose TTL is 0. */
            for(int di = 0; di < message.ancount; di++) {
                if(message.an[di].type == QTYPE_PTR && message.an[di].ttl == 0 &&
                   message.an[di].known.ptr.name &&
                   strcmp(message.an[di].known.ptr.name, instanceName) == 0)
                    return true;
            }
            const struct resource *goodbyeRecord =
                findAnyNamedRecord(&message, instanceName);
            if(!goodbyeRecord)
                return false;
            return ((goodbyeRecord->clazz & 0x7FFF) == QCLASS_IN &&
                    goodbyeRecord->ttl == 0);
        }
        return false;
    }
    if(expectation->requireSrv && !srvRecord)
        return false;

    if((txtRecord->clazz & 0x7FFF) != QCLASS_IN) {
        return false;
    }
    if(txtQuestion)
        if((txtQuestion->clazz & 0x7FFF) != QCLASS_IN) {
            return false;
        }
    if(expectation->checkTtl) {
        if(txtRecord->ttl != expectation->ttl)
            return false;
    } else {
        if(txtRecord->ttl == 0)
            return false;
    }

    if(expectation->requireSrv) {
        if((srvRecord->clazz & 0x7FFF) != QCLASS_IN)
            return false;
        if(srvQuestion)
            if((srvQuestion->clazz & 0x7FFF) != QCLASS_IN)
                return false;
        if(expectation->checkTtl) {
            if(srvRecord->ttl != expectation->ttl)
                return false;
        } else {
            if(srvRecord->ttl == 0)
                return false;
        }
        if(srvRecord->known.srv.priority != 0 ||
           srvRecord->known.srv.weight != 0 ||
           srvRecord->known.srv.port != expectation->port ||
           !srvRecord->known.srv.name ||
           !stringHasSuffix(srvRecord->known.srv.name, ".local."))
            return false;
    }

    return txtRecordMatchesExpectation(txtRecord, expectation);
}

static void
copyBuiltPacket(struct message *msg, UA_ByteString *outPacket) {
    int packetLen = message_packet_len(msg);
    unsigned char *packet = message_packet(msg);
    ck_assert_int_gt(packetLen, 0);
    ck_assert_ptr_ne(packet, NULL);
    ck_assert_uint_eq(UA_ByteString_allocBuffer(outPacket, (size_t)MAX_PACKET_LEN),
                      UA_STATUSCODE_GOOD);
    memcpy(outPacket->data, packet, (size_t)packetLen);
    outPacket->length = (size_t)packetLen;
}

static UA_Boolean
copyServiceInstanceNameFromPacket(const UA_ByteString *packet,
                                  const MdnsMessageExpectation *expectation,
                                  char *out, size_t outSize) {
    if(!packet || packet->length == 0 || packet->length >= MAX_PACKET_LEN)
        return false;

    unsigned char packetBuf[MAX_PACKET_LEN];
    memset(packetBuf, 0, sizeof(packetBuf));
    memcpy(packetBuf, packet->data, packet->length);

    struct message message;
    memset(&message, 0, sizeof(message));
    ck_assert_int_eq(message_parse(&message, packetBuf), 0);

    const char *instanceName = findServiceInstanceName(&message, expectation);
    if(!instanceName)
        return false;

    int written = snprintf(out, outSize, "%s", instanceName);
    ck_assert_int_gt(written, 0);
    ck_assert_int_lt(written, (int)outSize);
    return true;
}

static void
waitForMdnsServiceInstanceName(TestUdpIntercept *const *intercepts,
                               size_t interceptsSize, size_t previousSendCount,
                               const MdnsMessageExpectation *expectation,
                               char *out, size_t outSize) {
    for(size_t i = 0; i < 1000; i++) {
        iterateDiscoveryServers(2);
        if(getMaxInterceptedMdnsSequenceFor(intercepts, interceptsSize) <= previousSendCount)
            continue;

        for(size_t j = 0; j < interceptsSize; j++) {
            if(!intercepts[j] || intercepts[j]->sentNonEmptyMessages == 0)
                continue;
            for(size_t k = 0; k < TEST_UDP_CAPTURED_MESSAGES; k++) {
                if(intercepts[j]->recentMessageSequence[k] <= previousSendCount)
                    continue;
                if(copyServiceInstanceNameFromPacket(&intercepts[j]->recentMessages[k],
                                                    expectation, out, outSize))
                    return;
            }
        }
    }

    ck_abort_msg("Did not find service instance name for %s", expectation->serverName);
}

static UA_Boolean
mdnsPacketContainsQuestions(const UA_ByteString *packet, const char *name,
                            unsigned short firstType, unsigned short secondType) {
    if(!packet || packet->length == 0 || packet->length >= MAX_PACKET_LEN)
        return false;

    unsigned char packetBuf[MAX_PACKET_LEN];
    memset(packetBuf, 0, sizeof(packetBuf));
    memcpy(packetBuf, packet->data, packet->length);

    struct message message;
    memset(&message, 0, sizeof(message));
    ck_assert_int_eq(message_parse(&message, packetBuf), 0);

    if(message.header.qr != 0)
        return false;

    const struct question *first = findNamedQuestion(&message, name, firstType);
    const struct question *second = findNamedQuestion(&message, name, secondType);
    if(!first || !second)
        return false;

    ck_assert_uint_eq(first->clazz & 0x7FFF, QCLASS_IN);
    ck_assert_uint_eq(second->clazz & 0x7FFF, QCLASS_IN);
    return true;
}

static void
waitForMdnsQueryQuestions(TestUdpIntercept *const *intercepts, size_t interceptsSize,
                          size_t previousSendCount, const char *name,
                          unsigned short firstType, unsigned short secondType) {
    for(size_t i = 0; i < 1000; i++) {
        iterateDiscoveryServers(1);
        if(getMaxInterceptedMdnsSequenceFor(intercepts, interceptsSize) <= previousSendCount)
            continue;

        for(size_t j = 0; j < interceptsSize; j++) {
            if(!intercepts[j] || intercepts[j]->sentNonEmptyMessages == 0)
                continue;
            for(size_t k = 0; k < TEST_UDP_CAPTURED_MESSAGES; k++) {
                if(intercepts[j]->recentMessageSequence[k] <= previousSendCount)
                    continue;
                if(mdnsPacketContainsQuestions(&intercepts[j]->recentMessages[k], name,
                                              firstType, secondType))
                    return;
            }
        }
    }

    ck_abort_msg("Did not intercept mDNS query questions for %s", name);
}

static void
waitForMdnsQueryQuestionsOnServer(UA_Server *s,
                                  TestUdpIntercept *const *intercepts,
                                  size_t interceptsSize,
                                  size_t previousSendCount, const char *name,
                                  unsigned short firstType,
                                  unsigned short secondType) {
    for(size_t i = 0; i < 1000; i++) {
        UA_fakeSleep(1);
        UA_Server_run_iterate(s, false);
        if(getMaxInterceptedMdnsSequenceFor(intercepts, interceptsSize) <= previousSendCount)
            continue;

        for(size_t j = 0; j < interceptsSize; j++) {
            if(!intercepts[j] || intercepts[j]->sentNonEmptyMessages == 0)
                continue;
            for(size_t k = 0; k < TEST_UDP_CAPTURED_MESSAGES; k++) {
                if(intercepts[j]->recentMessageSequence[k] <= previousSendCount)
                    continue;
                if(mdnsPacketContainsQuestions(&intercepts[j]->recentMessages[k], name,
                                               firstType, secondType))
                    return;
            }
        }
    }

    ck_abort_msg("Did not intercept mDNS query questions for %s", name);
}

static void
buildInjectedMdnsPacket(const char *serviceInstance, const char *targetHost,
                        UA_UInt16 port, const char *path, const char *caps,
                        UA_Boolean includeTxt, UA_UInt32 txtTtl,
                        UA_Boolean includeSrv, UA_UInt32 srvTtl,
                        UA_ByteString *outPacket) {
    xht_t *txt = NULL;
    unsigned char *txtRaw = NULL;
    int txtRawLen = 0;
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.qr = 1;
    msg.header.aa = 1;

    if(includeTxt) {
        txt = xht_new(11);
        ck_assert_ptr_ne(txt, NULL);
        xht_store(txt, "path", 4, (void*)(uintptr_t)path, (int)strlen(path));
        xht_store(txt, "caps", 4, (void*)(uintptr_t)caps, (int)strlen(caps));
        txtRaw = sd2txt(txt, &txtRawLen);
        ck_assert_ptr_ne(txtRaw, NULL);
    }

    if(includeTxt) {
        message_an(&msg, (char*)(uintptr_t)serviceInstance, QTYPE_TXT, QCLASS_IN, txtTtl);
        message_rdata_raw(&msg, txtRaw, (unsigned short)txtRawLen);
    }

    if(includeSrv) {
        message_an(&msg, (char*)(uintptr_t)serviceInstance, QTYPE_SRV, QCLASS_IN, srvTtl);
        message_rdata_srv(&msg, 0, 0, port, (char*)(uintptr_t)targetHost);
    }

    copyBuiltPacket(&msg, outPacket);

    if(txtRaw)
        free(txtRaw);
    if(txt)
        xht_free(txt);
}

static void
buildInjectedPtrPacket(const char *serviceInstance, UA_UInt32 ttl,
                       UA_ByteString *outPacket) {
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.qr = 1;
    msg.header.aa = 1;

    message_an(&msg, "_opcua-tcp._tcp.local.", QTYPE_PTR, QCLASS_IN, ttl);
    message_rdata_name(&msg, (char*)(uintptr_t)serviceInstance);
    copyBuiltPacket(&msg, outPacket);
}

static void
buildInjectedTxtPacketWithClass(const char *serviceInstance, const char *path,
                                const char *caps, unsigned short clazz,
                                UA_UInt32 ttl, UA_ByteString *outPacket) {
    xht_t *txt = xht_new(11);
    ck_assert_ptr_ne(txt, NULL);
    xht_store(txt, "path", 4, (void*)(uintptr_t)path, (int)strlen(path));
    xht_store(txt, "caps", 4, (void*)(uintptr_t)caps, (int)strlen(caps));

    int txtRawLen = 0;
    unsigned char *txtRaw = sd2txt(txt, &txtRawLen);
    ck_assert_ptr_ne(txtRaw, NULL);

    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.qr = 1;
    msg.header.aa = 1;
    message_an(&msg, (char*)(uintptr_t)serviceInstance, QTYPE_TXT, clazz, ttl);
    message_rdata_raw(&msg, txtRaw, (unsigned short)txtRawLen);
    copyBuiltPacket(&msg, outPacket);

    free(txtRaw);
    xht_free(txt);
}

static void
buildInjectedARecordPacket(const char *name, unsigned short clazz,
                           UA_UInt32 ttl, UA_ByteString *outPacket) {
    static const unsigned char ipv4Raw[4] = {198, 51, 100, 7};
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.qr = 1;
    msg.header.aa = 1;
    message_an(&msg, (char*)(uintptr_t)name, QTYPE_A, clazz, ttl);
    message_rdata_raw(&msg, (unsigned char*)(uintptr_t)ipv4Raw, 4);
    copyBuiltPacket(&msg, outPacket);
}

static void
injectMdnsPacket(UA_ConnectionManager *cm, const UA_ByteString *packet) {
    UA_KeyValuePair params[2];
    UA_KeyValueMap paramsMap = {2, params};
    UA_String remoteAddress = UA_STRING("192.0.2.1");
    UA_UInt16 remotePort = 5353;

    params[0].key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_Variant_setScalar(&params[0].value, &remoteAddress,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "remote-port");
    UA_Variant_setScalar(&params[1].value, &remotePort,
                         &UA_TYPES[UA_TYPES_UINT16]);

    ck_assert_uint_eq(TestConnectionManager_inject(cm, TEST_MDNS_RECV_CONNECTION_ID,
                                                   UA_CONNECTIONSTATE_ESTABLISHED,
                                                   &paramsMap, packet),
                      UA_STATUSCODE_GOOD);
}

static void
waitForMdnsMessageAndAssert(TestUdpIntercept *const *intercepts,
                            size_t interceptsSize, size_t previousSendCount,
                            const MdnsMessageExpectation *expectation) {
    for(size_t i = 0; i < 1000; i++) {
        iterateDiscoveryServers(1);
        if(getMaxInterceptedMdnsSequenceFor(intercepts, interceptsSize) <= previousSendCount)
            continue;

        for(size_t j = 0; j < interceptsSize; j++) {
            if(!intercepts[j] || intercepts[j]->sentNonEmptyMessages == 0)
                continue;
            for(size_t k = 0; k < TEST_UDP_CAPTURED_MESSAGES; k++) {
                if(intercepts[j]->recentMessageSequence[k] <= previousSendCount)
                    continue;
                if(mdnsPacketMatchesExpectation(&intercepts[j]->recentMessages[k],
                                               expectation))
                    return;
            }
        }
    }

    ck_abort_msg("Did not intercept the expected mDNS message for %s",
                 expectation->serverName);
}

static UA_Boolean
hasMdnsMessageMatching(TestUdpIntercept *const *intercepts,
                       size_t interceptsSize, size_t previousSendCount,
                       const MdnsMessageExpectation *expectation) {
    for(size_t j = 0; j < interceptsSize; j++) {
        if(!intercepts[j] || intercepts[j]->sentNonEmptyMessages == 0)
            continue;
        for(size_t k = 0; k < TEST_UDP_CAPTURED_MESSAGES; k++) {
            if(intercepts[j]->recentMessageSequence[k] <= previousSendCount)
                continue;
            if(mdnsPacketMatchesExpectation(&intercepts[j]->recentMessages[k],
                                            expectation))
                return true;
        }
    }
    return false;
}

static void
replaceUdpConnectionManager(UA_Server *s) {
    UA_EventLoop *el = UA_Server_getConfig(s)->eventLoop;
    UA_String udpName = UA_STRING("udp");
    UA_String udpOriginalName = UA_STRING("udp-original");

    for(UA_EventSource *es = el->eventSources; es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;

        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&cm->protocol, &udpName))
            continue;
        /* Keep the default UDP manager intact to avoid lifecycle warnings in
         * the event loop. Renaming its protocol ensures discovery selects the
         * test connection manager below for "udp". */
        cm->protocol = udpOriginalName;
        break;
    }

    testUdpCm = TestConnectionManager_new("udp", &interceptingUdpOverloads);
    ck_assert_ptr_ne(testUdpCm, NULL);
    testUdpIntercept = (TestUdpIntercept*)UA_calloc(1, sizeof(TestUdpIntercept));
    ck_assert_ptr_ne(testUdpIntercept, NULL);
    TestConnectionManager_setContext(testUdpCm, testUdpIntercept);

    UA_Boolean logEvents = true;
    UA_StatusCode retval =
        UA_KeyValueMap_setScalar(&testUdpCm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "log-events"),
                                 &logEvents,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    el->registerEventSource(el, &testUdpCm->eventSource);
}

static void
replaceUdpConnectionManagerFor(UA_Server *s, UA_ConnectionManager **outTestCm,
                               TestUdpIntercept **outIntercept) {
    UA_EventLoop *el = UA_Server_getConfig(s)->eventLoop;
    UA_String udpName = UA_STRING("udp");
    UA_String udpOriginalName = UA_STRING("udp-original");

    for(UA_EventSource *es = el->eventSources; es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;

        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&cm->protocol, &udpName))
            continue;
        cm->protocol = udpOriginalName;
        break;
    }

    UA_ConnectionManager *newTestCm =
        TestConnectionManager_new("udp", &interceptingUdpOverloads);
    ck_assert_ptr_ne(newTestCm, NULL);
    TestUdpIntercept *newIntercept =
        (TestUdpIntercept*)UA_calloc(1, sizeof(TestUdpIntercept));
    ck_assert_ptr_ne(newIntercept, NULL);
    TestConnectionManager_setContext(newTestCm, newIntercept);

    UA_Boolean logEvents = true;
    UA_StatusCode retval =
        UA_KeyValueMap_setScalar(&newTestCm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "log-events"),
                                 &logEvents,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    el->registerEventSource(el, &newTestCm->eventSource);
    *outTestCm = newTestCm;
    *outIntercept = newIntercept;
}

static void
setup_server(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->serversOnNetworkEnabled = true;
    config->discoveryNotificationCallback = serverDiscoveryNotificationCallback;
    globalInterceptedMdnsMessages = 0;
    resetDiscoveryCounters();

    replaceUdpConnectionManager(server);
    addMdnsDriver(server, true, true);

    UA_StatusCode retval = UA_Server_run_startup(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    const char *capabilities[] = {"NA"};
    registerServerOnNetwork(server, "LDS_mdnsd_test", "opc.tcp://localhost:4840",
                            capabilities, 1);
}

static void
teardown_server(void) {
    if(!server)
        return;

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
    testUdpCm = NULL;
    clearTestUdpIntercept(&testUdpIntercept);
}

static void
serverDiscoveryNotificationCallback(UA_Server *server,
                                    UA_ApplicationNotificationType type,
                                    const UA_KeyValueMap payload) {
    (void)server;
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK)
        return;

    const UA_ServerOnNetwork *serverOnNetwork = (const UA_ServerOnNetwork*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-on-network"),
                                 &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    const UA_Boolean *serverAdded = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-added"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *serverRemoved = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-removed"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *serverUpdated = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-updated"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* Server-thread code path -- record failures instead of ck_assert
     * (see threadCallbackFailures) */
    if(!serverOnNetwork || !serverAdded || !serverRemoved || !serverUpdated) {
        recordThreadCallbackFailure("serverDiscoveryNotificationCallback: "
                                    "incomplete notification payload");
        return;
    }

    DiscoveryIntegrationCounters *counters = &discoveryCounters;
    UA_Boolean isServerAnnounce = (*serverAdded || *serverUpdated);
    UA_Boolean isTxtReceived = serverOnNetworkHasTxtData(serverOnNetwork);

    counters->serverOnNetworkCalls++;
    counters->lastIsServerAnnounce = isServerAnnounce;
    counters->lastIsTxtReceived = isTxtReceived;
    if(isServerAnnounce)
        counters->serverOnNetworkAnnounceCalls++;
    if(*serverRemoved)
        counters->serverOnNetworkRemoveCalls++;

    copyUAStringToCString(&serverOnNetwork->serverName,
                          counters->lastServerName,
                          sizeof(counters->lastServerName));
    copyUAStringToCString(&serverOnNetwork->discoveryUrl,
                          counters->lastDiscoveryUrl,
                          sizeof(counters->lastDiscoveryUrl));
    joinCapabilities(serverOnNetwork, counters->lastCapabilities,
                     sizeof(counters->lastCapabilities));
}

THREAD_CALLBACK(serverloop_lds_public) {
    while(*runningLds)
        UA_Server_run_iterate(serverLds, true);
    return 0;
}

THREAD_CALLBACK(serverloop_register_public) {
    while(*runningRegister)
        UA_Server_run_iterate(serverRegister, true);
    return 0;
}

static void
iterateDiscoveryServers(size_t iterations) {
    for(size_t i = 0; i < iterations; i++) {
        UA_fakeSleep(1000);
        /* The dedicated server threads (serverThreadLds, serverThreadRegister)
         * run a blocking iterate and drive the EventLoop continuously. We must
         * not call iterate from the test thread concurrently -- the EventLoop
         * mutex is not held across the blocking select, so a second caller
         * re-enters the run method and logs "Cannot run EventLoop from the
         * run method itself", which also prevents event delivery. Just sleep
         * to give the server threads time to process the fake-clock advance. */
#ifndef UA_ARCHITECTURE_WIN32
        struct timespec ts = {0, 1000000}; /* 1ms */
        nanosleep(&ts, NULL);
#else
        Sleep(1);
#endif
    }
}

static void
waitForServerOnNetworkCalls(UA_UInt32 expectedCalls, UA_UInt32 timeoutMs) {
    for(UA_UInt32 i = 0;
        i < timeoutMs && discoveryCounters.serverOnNetworkCalls < expectedCalls;
        i++) {
        UA_fakeSleep(1);
#ifndef UA_ARCHITECTURE_WIN32
        struct timespec ts = {0, 1000000}; /* 1ms */
        nanosleep(&ts, NULL);
#else
        Sleep(1);
#endif
    }
}

static void
setup_public_api_servers(void) {
    memset(&discoveryCounters, 0, sizeof(discoveryCounters));
    testUdpCmLds = NULL;
    testUdpCmRegister = NULL;
    testUdpInterceptLds = NULL;
    testUdpInterceptRegister = NULL;
    globalInterceptedMdnsMessages = 0;

    UA_ServerConfig ldsConfig;
    memset(&ldsConfig, 0, sizeof(ldsConfig));
    ck_assert_uint_eq(UA_ServerConfig_setMinimal(&ldsConfig, 4840, NULL),
                      UA_STATUSCODE_GOOD);
    serverLds = UA_Server_newWithConfig(&ldsConfig);
    ck_assert_ptr_ne(serverLds, NULL);

    UA_ServerConfig *ldsServerConfig = UA_Server_getConfig(serverLds);
    ldsServerConfig->tcpReuseAddr = true;
    ldsServerConfig->serversOnNetworkEnabled = true;
    ldsServerConfig->applicationDescription.applicationType =
        UA_APPLICATIONTYPE_DISCOVERYSERVER;
    ldsServerConfig->discoveryNotificationCallback =
        serverDiscoveryNotificationCallback;

    replaceUdpConnectionManagerFor(serverLds, &testUdpCmLds, &testUdpInterceptLds);
    addMdnsDriver(serverLds, true, true);

    ck_assert_uint_eq(UA_Server_run_startup(serverLds), UA_STATUSCODE_GOOD);

    const char *ldsCaps[] = {"LDS", "MyFancyCap"};
    registerServerOnNetwork(serverLds, "LDS_public_api", "opc.tcp://localhost:4840",
                            ldsCaps, 2);

    runningLds = UA_Boolean_new();
    *runningLds = true;
    THREAD_CREATE(serverThreadLds, serverloop_lds_public);

    UA_ServerConfig registerConfig;
    memset(&registerConfig, 0, sizeof(registerConfig));
    ck_assert_uint_eq(UA_ServerConfig_setMinimal(&registerConfig, 16664, NULL),
                      UA_STATUSCODE_GOOD);
    serverRegister = UA_Server_newWithConfig(&registerConfig);
    ck_assert_ptr_ne(serverRegister, NULL);

    UA_ServerConfig *registerServerConfig = UA_Server_getConfig(serverRegister);
    registerServerConfig->tcpReuseAddr = true;
    registerServerConfig->serversOnNetworkEnabled = true;
    UA_String_clear(&registerServerConfig->applicationDescription.applicationUri);
    registerServerConfig->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.test.server_register_public_api");

    replaceUdpConnectionManagerFor(serverRegister, &testUdpCmRegister,
                                   &testUdpInterceptRegister);
    addMdnsDriver(serverRegister, true, true);

    ck_assert_uint_eq(UA_Server_run_startup(serverRegister), UA_STATUSCODE_GOOD);

    const char *registerCaps[] = {"NA"};
    registerServerOnNetwork(serverRegister, "Register_public_api",
                            "opc.tcp://localhost:16664", registerCaps, 1);

    runningRegister = UA_Boolean_new();
    *runningRegister = true;
    THREAD_CREATE(serverThreadRegister, serverloop_register_public);
}

static void
teardown_public_api_servers(void) {
    if(serverRegister) {
        *runningRegister = false;
        THREAD_JOIN(serverThreadRegister);
    }

    if(serverLds) {
        *runningLds = false;
        THREAD_JOIN(serverThreadLds);
    }

    /* The mdnsd backend shares its multicast sockets globally. Shut down the
     * LDS server first because it owns the sockets, otherwise the register
     * server waits forever for the shared mDNS connections to disappear. */
    if(serverRegister && serverLds) {
        UA_Server_run_shutdown(serverLds);
        UA_Server_run_shutdown(serverRegister);
    } else if(serverRegister) {
        UA_Server_run_shutdown(serverRegister);
    } else if(serverLds) {
        UA_Server_run_shutdown(serverLds);
    }

    if(serverRegister) {
        UA_Boolean_delete(runningRegister);
        runningRegister = NULL;
        UA_Server_delete(serverRegister);
        serverRegister = NULL;
    }

    if(serverLds) {
        UA_Boolean_delete(runningLds);
        runningLds = NULL;
        UA_Server_delete(serverLds);
        serverLds = NULL;
    }

    memset(&discoveryCounters, 0, sizeof(discoveryCounters));
    testUdpCmLds = NULL;
    testUdpCmRegister = NULL;
    clearTestUdpIntercept(&testUdpInterceptLds);
    clearTestUdpIntercept(&testUdpInterceptRegister);
}

static void
setDiscoveryTestClientDefaults(UA_ClientConfig *cc) {
    UA_ClientConfig_setDefault(cc);
    cc->securityMode = UA_MESSAGESECURITYMODE_NONE;
}

static void
registerWithLdsPublicApi(void) {
    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(cc));
    setDiscoveryTestClientDefaults(&cc);

    *runningRegister = false;
    THREAD_JOIN(serverThreadRegister);

    UA_StatusCode retval =
        UA_Server_registerDiscovery(serverRegister, &cc,
                                    UA_STRING("opc.tcp://localhost:4840"),
                                    UA_STRING_NULL);

    *runningRegister = true;
    THREAD_CREATE(serverThreadRegister, serverloop_register_public);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void
deregisterFromLdsPublicApi(void) {
    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(cc));
    setDiscoveryTestClientDefaults(&cc);

    *runningRegister = false;
    THREAD_JOIN(serverThreadRegister);

    UA_StatusCode retval =
        UA_Server_deregisterDiscovery(serverRegister, &cc,
                                      UA_STRING("opc.tcp://localhost:4840"));

    *runningRegister = true;
    THREAD_CREATE(serverThreadRegister, serverloop_register_public);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static UA_Boolean
isServerRegisteredAtLds(void) {
    UA_Client *client = UA_Client_new();
    if(!client)
        return false;
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    setDiscoveryTestClientDefaults(cc);
    /* The default 5s timeout would stretch the for-loop into tens of
     * minutes when the discovery handshake never completes. Use a short
     * timeout so the test fails fast and reports a clear problem. */
    cc->timeout = 200;
    UA_ApplicationDescription *servers = NULL;
    size_t serversSize = 0;

    /* The dedicated server thread drives the LDS EventLoop continuously, so
     * we must not also call iterate from the test thread -- the EventLoop's
     * "executing" flag is already set and a second call would log an error
     * and return immediately. The server thread wakes from select() on
     * incoming connection data, so it handles the findServers request. */
    UA_StatusCode retval =
        UA_Client_findServers(client, "opc.tcp://localhost:4840",
                              0, NULL, 0, NULL,
                              &serversSize, &servers);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return false;
    }

    UA_String uri = UA_STRING("urn:open62541.test.server_register_public_api");
    UA_Boolean found = false;
    for(size_t i = 0; i < serversSize; i++) {
        if(UA_String_equal(&servers[i].applicationUri, &uri)) {
            found = true;
            break;
        }
    }

    UA_Array_delete(servers, serversSize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_delete(client);
    return found;
}

static UA_Boolean
serverNameHasPrefix(const UA_String *serverName, const char *prefix) {
    size_t prefixLen = strlen(prefix);
    return serverName && serverName->data &&
           serverName->length >= prefixLen &&
           memcmp(serverName->data, prefix, prefixLen) == 0;
}

static void
findServersOnNetworkAndCheck(const char *endpointUrl,
                             const char *expectedServerNamePrefixes[],
                             size_t expectedServerNamesSize,
                             const char **filterCapabilities,
                             size_t filterCapabilitiesSize) {
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_ne(client, NULL);
    setDiscoveryTestClientDefaults(UA_Client_getConfig(client));

    UA_ServerOnNetwork *serverOnNetwork = NULL;
    size_t serverOnNetworkSize = 0;
    UA_String *serverCapabilityFilter = NULL;

    if(filterCapabilitiesSize > 0) {
        serverCapabilityFilter =
            (UA_String*)UA_Array_new(filterCapabilitiesSize, &UA_TYPES[UA_TYPES_STRING]);
        ck_assert_ptr_ne(serverCapabilityFilter, NULL);
        for(size_t i = 0; i < filterCapabilitiesSize; i++)
            serverCapabilityFilter[i] = UA_String_fromChars(filterCapabilities[i]);
    }

    UA_StatusCode retval =
        UA_Client_findServersOnNetwork(client, endpointUrl, 0, 0,
                                       filterCapabilitiesSize, serverCapabilityFilter,
                                       &serverOnNetworkSize, &serverOnNetwork);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(serverOnNetworkSize, expectedServerNamesSize);

    for(size_t i = 0; i < expectedServerNamesSize; i++) {
        UA_Boolean found = false;
        for(size_t j = 0; j < serverOnNetworkSize; j++) {
            if(serverNameHasPrefix(&serverOnNetwork[j].serverName,
                                   expectedServerNamePrefixes[i])) {
                found = true;
                break;
            }
        }
        ck_assert_msg(found, "Expected %s in serverOnNetwork list, but not found",
                      expectedServerNamePrefixes[i]);
    }

    UA_Array_delete(serverCapabilityFilter, filterCapabilitiesSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(serverOnNetwork, serverOnNetworkSize,
                    &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    UA_Client_delete(client);
}

START_TEST(MdnsStartupTriggersSendPath) {
    ck_assert_ptr_ne(server->discoveryDriver, NULL);
    ck_assert_uint_eq(TestConnectionManager_getCounters(testUdpCm,
                                                        TEST_MDNS_RECV_CONNECTION_ID,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(testUdpCm,
                                                        TEST_MDNS_RECV_CONNECTION_ID + 1,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);

    TestUdpIntercept *intercepts[] = {testUdpIntercept};
    MdnsMessageExpectation expectation =
        {"LDS_mdnsd_test", "/", "NA", 4840, true, 600, true};

    for(size_t i = 0; i < 3; i++) {
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, false);
    }

    waitForMdnsMessageAndAssert(intercepts, 1, 0, &expectation);
}
END_TEST

static UA_Server *
createMdnsQueryTestServer(UA_ConnectionManager **outCm,
                          TestUdpIntercept **outIntercept,
                          UA_Boolean queryPresence,
                          UA_Boolean queryDetails,
                          UA_UInt32 queryInterval) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(localServer);
    config->serversOnNetworkEnabled = true;
    config->discoveryNotificationCallback = serverDiscoveryNotificationCallback;

    replaceUdpConnectionManagerFor(localServer, outCm, outIntercept);
    addMdnsDriverWithQueries(localServer, true, false, queryPresence,
                             queryDetails, queryInterval);
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);
    return localServer;
}

START_TEST(MdnsQueryPresenceSendsStartupPtrQuery) {
    UA_ConnectionManager *localCm = NULL;
    TestUdpIntercept *localIntercept = NULL;
    size_t initialSends = globalInterceptedMdnsMessages;
    UA_Server *localServer =
        createMdnsQueryTestServer(&localCm, &localIntercept, true, false, 0);
    (void)localCm;

    TestUdpIntercept *intercepts[] = {localIntercept};
    waitForMdnsQueryQuestionsOnServer(localServer, intercepts, 1, initialSends,
                                      "_opcua-tcp._tcp.local.",
                                      QTYPE_PTR, QTYPE_PTR);

    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
    clearTestUdpIntercept(&localIntercept);
}
END_TEST

START_TEST(MdnsQueryDetailsSendsSrvTxtQueriesForPtr) {
    UA_ConnectionManager *localCm = NULL;
    TestUdpIntercept *localIntercept = NULL;
    UA_Server *localServer =
        createMdnsQueryTestServer(&localCm, &localIntercept, false, true, 0);
    const char *serviceInstance =
        "QueryDetailsPtr-query-host._opcua-tcp._tcp.local.";
    UA_ByteString ptrPacket = UA_BYTESTRING_NULL;

    buildInjectedPtrPacket(serviceInstance, 600, &ptrPacket);
    size_t previousSendCount = globalInterceptedMdnsMessages;
    injectMdnsPacket(localCm, &ptrPacket);

    TestUdpIntercept *intercepts[] = {localIntercept};
    waitForMdnsQueryQuestionsOnServer(localServer, intercepts, 1,
                                      previousSendCount, serviceInstance,
                                      QTYPE_SRV, QTYPE_TXT);

    UA_ByteString_clear(&ptrPacket);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
    clearTestUdpIntercept(&localIntercept);
}
END_TEST

START_TEST(MdnsQueryDetailsSendsTxtQueryForSrvOnly) {
    UA_ConnectionManager *localCm = NULL;
    TestUdpIntercept *localIntercept = NULL;
    UA_Server *localServer =
        createMdnsQueryTestServer(&localCm, &localIntercept, false, true, 0);
    const char *serviceInstance =
        "QueryDetailsSrv-query-host._opcua-tcp._tcp.local.";
    UA_ByteString srvPacket = UA_BYTESTRING_NULL;

    buildInjectedMdnsPacket(serviceInstance, "query-host.local.", 4840,
                            NULL, NULL, false, 0, true, 600, &srvPacket);
    size_t previousSendCount = globalInterceptedMdnsMessages;
    injectMdnsPacket(localCm, &srvPacket);

    TestUdpIntercept *intercepts[] = {localIntercept};
    waitForMdnsQueryQuestionsOnServer(localServer, intercepts, 1,
                                      previousSendCount, serviceInstance,
                                      QTYPE_TXT, QTYPE_TXT);

    UA_ByteString_clear(&srvPacket);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
    clearTestUdpIntercept(&localIntercept);
}
END_TEST

START_TEST(MdnsQueryDetailsSendsSrvQueryForTxtOnly) {
    UA_ConnectionManager *localCm = NULL;
    TestUdpIntercept *localIntercept = NULL;
    UA_Server *localServer =
        createMdnsQueryTestServer(&localCm, &localIntercept, false, true, 0);
    const char *serviceInstance =
        "QueryDetailsTxt-query-host._opcua-tcp._tcp.local.";
    UA_ByteString txtPacket = UA_BYTESTRING_NULL;

    buildInjectedMdnsPacket(serviceInstance, "query-host.local.", 4840,
                            "/query", "DA", true, 600, false, 0, &txtPacket);
    size_t previousSendCount = globalInterceptedMdnsMessages;
    injectMdnsPacket(localCm, &txtPacket);

    TestUdpIntercept *intercepts[] = {localIntercept};
    waitForMdnsQueryQuestionsOnServer(localServer, intercepts, 1,
                                      previousSendCount, serviceInstance,
                                      QTYPE_SRV, QTYPE_SRV);

    UA_ByteString_clear(&txtPacket);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
    clearTestUdpIntercept(&localIntercept);
}
END_TEST

START_TEST(PublicApiFindServersOnNetworkListsRegisteredServers) {
    const char *expectedServerNames[1];
    const char *capsLds[] = {"LDS"};
    const char *capsNa[] = {"NA"};
    const char *capsMultipleNone[] = {"LDS", "NA"};
    const char *capsMultipleCustom[] = {"LDS", "MyFancyCap"};
    const char *capsMultipleCustomIgnoreCase[] = {"LDS", "myfancycap"};

    expectedServerNames[0] = "LDS_public_api";

    registerWithLdsPublicApi();
    for(size_t i = 0; i < 30 && !isServerRegisteredAtLds(); i++)
        iterateDiscoveryServers(1);
    ck_assert(isServerRegisteredAtLds());

    UA_fakeSleep(4000);
    iterateDiscoveryServers(1);

    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 expectedServerNames, 1, NULL, 0);
    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 expectedServerNames, 1, capsLds, 1);
    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 NULL, 0, capsNa, 1);
    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 NULL, 0, capsMultipleNone, 2);
    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 expectedServerNames, 1, capsMultipleCustom, 2);
    findServersOnNetworkAndCheck("opc.tcp://localhost:4840",
                                 expectedServerNames, 1,
                                 capsMultipleCustomIgnoreCase, 2);
}
END_TEST

START_TEST(PublicApiFindServersOnNetworkAppliesLimitAfterCapabilityFilter) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);

    UA_ServerConfig *localConfig = UA_Server_getConfig(localServer);
    localConfig->serversOnNetworkEnabled = true;
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

    const char *capsDa[] = {"DA"};
    const char *capsLds[] = {"LDS"};
    registerServerOnNetwork(localServer, "candidate-da",
                            "opc.tcp://localhost:4841", capsDa, 1);
    registerServerOnNetwork(localServer, "candidate-lds",
                            "opc.tcp://localhost:4842", capsLds, 1);

    UA_String filter = UA_STRING("LDS");
    UA_DateTime lastCounterResetTime = 0;
    size_t serverOnNetworkSize = 0;
    UA_ServerOnNetwork *serverOnNetwork = NULL;
    UA_StatusCode retval =
        UA_Server_findServersOnNetwork(localServer, UA_STRING_NULL, 0, 1, 1,
                                       &filter, &lastCounterResetTime,
                                       &serverOnNetworkSize, &serverOnNetwork);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(serverOnNetworkSize, 1);
    UA_String expectedName = UA_STRING("candidate-lds");
    ck_assert(UA_String_equal(&serverOnNetwork[0].serverName, &expectedName));

    UA_Array_delete(serverOnNetwork, serverOnNetworkSize,
                    &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
}
END_TEST

START_TEST(MdnsDriverParamCopiessettingsOnStartup) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);

    replaceUdpConnectionManager(localServer);

    /* Add a driver with the listening disabled and the announcement enabled
     * with a custom TTL. The driver must keep the parameters after startup. */
    UA_KeyValuePair params[2];
    UA_Boolean listen = false;
    UA_Boolean announce = true;
    UA_UInt32 announceTTL = 4242;
    params[0].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[0].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[1].key = UA_QUALIFIEDNAME(0, "announce-ttl");
    UA_Variant_setScalar(&params[1].value, &announceTTL, &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap paramsMap = {2, params};
    UA_MdnsDriver *mdns = UA_MdnsDriver_Mdnsd(paramsMap);
    ck_assert_ptr_ne(mdns, NULL);
    ck_assert_uint_eq(UA_Server_addDriver(localServer, &mdns->drv), UA_STATUSCODE_GOOD);

    UA_StatusCode retval = UA_Server_run_startup(localServer);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Driver *runningDrv = getMdnsDriver(localServer);
    ck_assert_ptr_ne(runningDrv, NULL);
    ck_assert_ptr_eq(runningDrv, &mdns->drv);
    const UA_Boolean *listenParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&runningDrv->params,
                                 UA_QUALIFIEDNAME(0, "listen"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_UInt32 *ttlParam = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&runningDrv->params,
                                 UA_QUALIFIEDNAME(0, "announce-ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(listenParam, NULL);
    ck_assert_ptr_ne(ttlParam, NULL);
    ck_assert(!*listenParam);
    ck_assert_uint_eq(*ttlParam, 4242);

    UA_Server_run_shutdown(localServer);
    UA_Server_delete(localServer);
}
END_TEST

START_TEST(MdnsShutdownSendsSelfGoodbyeAndDrainsQueue) {
    UA_Server *localServer = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(localServer, NULL);

    /* Enable the ServerOnNetwork table so UA_Server_registerServerOnNetwork
     * does not return BadNotImplemented. */
    UA_ServerConfig *localConfig = UA_Server_getConfig(localServer);
    localConfig->serversOnNetworkEnabled = true;

    UA_ConnectionManager *localTestCm = NULL;
    TestUdpIntercept *localIntercept = NULL;
    replaceUdpConnectionManagerFor(localServer, &localTestCm, &localIntercept);
    addMdnsDriver(localServer, true, true);
    ck_assert_uint_eq(UA_Server_run_startup(localServer), UA_STATUSCODE_GOOD);

    /* Register a local server so the driver will announce and later retract
     * it. The capability "NA" matches the default that the driver encodes
     * when no capabilities are provided. */
    const char *caps[] = {"NA"};
    registerServerOnNetwork(localServer, "LDS_mdnsd_shutdown",
                            "opc.tcp://localhost:4840", caps, 1);

    /* Let the startup announce fully settle before tearing the server down. */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(localServer, false);

    TestUdpIntercept *intercepts[] = {localIntercept};
    size_t previousSendCount = globalInterceptedMdnsMessages;
    MdnsMessageExpectation expectation =
        {"LDS_mdnsd_shutdown", "/", "NA", 4840, false, 0, true};

    UA_Server_run_shutdown(localServer);
    waitForMdnsMessageAndAssert(intercepts, 1, previousSendCount, &expectation);

    size_t sendsAfterShutdown = getInterceptedMdnsSendCountFor(intercepts, 1);
    for(size_t i = 0; i < 5; i++)
        UA_fakeSleep(1000);
    ck_assert_uint_eq(getInterceptedMdnsSendCountFor(intercepts, 1),
                      sendsAfterShutdown);

    UA_Server_delete(localServer);
    localTestCm = NULL;
    clearTestUdpIntercept(&localIntercept);
}
END_TEST

START_TEST(MdnsUpdateOnlineOfflineTriggersSendPath) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)server->discoveryDriver;
    ck_assert_ptr_ne(dm, NULL);
    TestUdpIntercept *intercepts[] = {testUdpIntercept};
    size_t previousSendCount = globalInterceptedMdnsMessages;
    MdnsMessageExpectation onlineExpectation =
        {"RemoteTestServer", "/", "NA", 16664, true, 0, false};

    updateMdnsForDiscoveryUrl(server, "RemoteTestServer",
                              "opc.tcp://localhost:16664", true);
    waitForMdnsMessageAndAssert(intercepts, 1, previousSendCount,
                                &onlineExpectation);

    updateMdnsForDiscoveryUrl(server, "RemoteTestServer",
                              "opc.tcp://localhost:16664", false);
}
END_TEST

START_TEST(PublicApiRegisterDeregisterCallback) {
    registerWithLdsPublicApi();

    for(size_t i = 0; i < 30 && !isServerRegisteredAtLds(); i++)
        iterateDiscoveryServers(1);

    ck_assert(isServerRegisteredAtLds());

    deregisterFromLdsPublicApi();

    for(size_t i = 0; i < 30 && isServerRegisteredAtLds(); i++)
        iterateDiscoveryServers(1);

    ck_assert(!isServerRegisteredAtLds());
}
END_TEST

START_TEST(PublicApiDeregisterDiscoveryKeepsLocalMdnsRecord) {
    registerWithLdsPublicApi();
    for(size_t i = 0; i < 30 && !isServerRegisteredAtLds(); i++)
        iterateDiscoveryServers(1);
    ck_assert(isServerRegisteredAtLds());
    ck_assert_uint_eq(countServersOnNetworkByName(serverRegister,
                                                  "Register_public_api"), 1);

    deregisterFromLdsPublicApi();

    for(size_t i = 0; i < 30 && isServerRegisteredAtLds(); i++)
        iterateDiscoveryServers(1);
    ck_assert(!isServerRegisteredAtLds());
    ck_assert_uint_eq(countServersOnNetworkByName(serverRegister,
                                                  "Register_public_api"), 1);
}
END_TEST

START_TEST(PublicApiInjectedTxtThenSrvTriggersAnnounceCallback) {
    const char *serverName = "InjectedTxtFirst";
    const char *hostname = "remote-host";
    char serviceInstance[128];
    char expectedServerName[128];
    char targetHost[64];
    UA_ByteString txtPacket = UA_BYTESTRING_NULL;
    UA_ByteString srvPacket = UA_BYTESTRING_NULL;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedServerOnNetworkName(expectedServerName, sizeof(expectedServerName),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7777, "/custom",
                            "DA,LDS", true, 600, false, 0, &txtPacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7777, "/custom",
                            "DA,LDS", false, 0, true, 600, &srvPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &txtPacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);

    injectMdnsPacket(testUdpCmLds, &srvPacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkRemoveCalls, 0);
    ck_assert(discoveryCounters.lastIsServerAnnounce);
    ck_assert(discoveryCounters.lastIsTxtReceived);
    ck_assert_str_eq(discoveryCounters.lastServerName, expectedServerName);
    ck_assert_str_eq(discoveryCounters.lastDiscoveryUrl,
                     "opc.tcp://remote-host.local:7777/custom");
    ck_assert_str_eq(discoveryCounters.lastCapabilities, "DA,LDS");

    UA_ByteString_clear(&txtPacket);
    UA_ByteString_clear(&srvPacket);
}
END_TEST

START_TEST(PublicApiInjectedSrvThenTxtUpdatesCallbackState) {
    const char *serverName = "InjectedSrvFirst";
    const char *hostname = "late-txt-host";
    char serviceInstance[128];
    char expectedServerName[128];
    char targetHost[64];
    UA_ByteString srvPacket = UA_BYTESTRING_NULL;
    UA_ByteString txtPacket = UA_BYTESTRING_NULL;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedServerOnNetworkName(expectedServerName, sizeof(expectedServerName),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 8888, "/late",
                            "SV", false, 0, true, 600, &srvPacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 8888, "/late",
                            "SV", true, 600, false, 0, &txtPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &srvPacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);

    injectMdnsPacket(testUdpCmLds, &txtPacket);
    waitForServerOnNetworkCalls(1, 1500);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 1);
    ck_assert(discoveryCounters.lastIsTxtReceived);
    ck_assert_str_eq(discoveryCounters.lastServerName, expectedServerName);
    ck_assert_str_eq(discoveryCounters.lastDiscoveryUrl,
                     "opc.tcp://late-txt-host.local:8888/late");
    ck_assert_str_eq(discoveryCounters.lastCapabilities, "SV");

    UA_ByteString_clear(&srvPacket);
    UA_ByteString_clear(&txtPacket);
}
END_TEST

START_TEST(PublicApiInjectedTtlZeroRemovesRemoteServer) {
    const char *serverName = "InjectedRemove";
    const char *hostname = "remove-host";
    char serviceInstance[128];
    char expectedServerName[128];
    char targetHost[64];
    UA_ByteString announcePacket = UA_BYTESTRING_NULL;
    UA_ByteString goodbyePacket = UA_BYTESTRING_NULL;
    TestUdpIntercept *intercepts[] = {testUdpInterceptLds};
    MdnsMessageExpectation remoteGoodbyeExpectation =
        {serverName, "/gone", "RM", 9999, true, 0, true};

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedServerOnNetworkName(expectedServerName, sizeof(expectedServerName),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 9999, "/gone",
                            "RM", true, 600, true, 600, &announcePacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 9999, "/gone",
                            "RM", true, 0, false, 0, &goodbyePacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &announcePacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 1);

    size_t sendsAfterAnnounce =
        getMaxInterceptedMdnsSequenceFor(intercepts, 1);

    injectMdnsPacket(testUdpCmLds, &goodbyePacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 2);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkRemoveCalls, 1);
    ck_assert(!discoveryCounters.lastIsServerAnnounce);
    ck_assert(discoveryCounters.lastIsTxtReceived);
    ck_assert_str_eq(discoveryCounters.lastServerName, expectedServerName);
    ck_assert_str_eq(discoveryCounters.lastDiscoveryUrl,
                     "opc.tcp://remove-host.local:9999/gone");
    ck_assert(!hasMdnsMessageMatching(intercepts, 1, sendsAfterAnnounce,
                                      &remoteGoodbyeExpectation));

    UA_ByteString_clear(&announcePacket);
    UA_ByteString_clear(&goodbyePacket);
}
END_TEST

START_TEST(PublicApiInjectedRemoteServerIsNotMirroredBackOut) {
    const char *serverName = "InjectedNoMirror";
    const char *hostname = "remote-no-mirror-host";
    char serviceInstance[128];
    char targetHost[64];
    UA_ByteString announcePacket = UA_BYTESTRING_NULL;
    size_t previousSendCount = globalInterceptedMdnsMessages;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7776, "/mirror", "DA",
                            true, 600, true, 600, &announcePacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &announcePacket);
    waitForServerOnNetworkCalls(1, 1500);

    ck_assert_uint_eq(globalInterceptedMdnsMessages, previousSendCount);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);

    UA_ByteString_clear(&announcePacket);
}
END_TEST

START_TEST(PublicApiInjectedPtrIsIgnoredWithoutDetails) {
    const char *serverName = "InjectedPtr";
    const char *hostname = "ptr-host";
    char serviceInstance[128];
    UA_ByteString ptrPacket = UA_BYTESTRING_NULL;
    size_t previousSendCount = globalInterceptedMdnsMessages;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    buildInjectedPtrPacket(serviceInstance, 600, &ptrPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &ptrPacket);
    /* The mdnsd driver does not implement SRV/TXT queries for incomplete
     * entries (no detail querying), so the PTR record alone should not
     * trigger a callback. */
    iterateDiscoveryServers(2);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);
    ck_assert_uint_eq(globalInterceptedMdnsMessages, previousSendCount);

    UA_ByteString_clear(&ptrPacket);
}
END_TEST

START_TEST(PublicApiInjectedPtrDoesNotEmitQueryWhenDetailsArrive) {
    const char *serverName = "InjectedPtrNoQuery";
    const char *hostname = "ptr-no-query-host";
    char serviceInstance[128];
    char targetHost[64];
    UA_ByteString ptrPacket = UA_BYTESTRING_NULL;
    UA_ByteString announcePacket = UA_BYTESTRING_NULL;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedPtrPacket(serviceInstance, 600, &ptrPacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7777, "/timely", "DA",
                            true, 600, true, 600, &announcePacket);

    size_t previousSendCount = globalInterceptedMdnsMessages;
    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &ptrPacket);
    injectMdnsPacket(testUdpCmLds, &announcePacket);
    iterateDiscoveryServers(2);

    /* The mdnsd driver is purely passive: it should never issue a query
     * in response to a PTR record. The callback is fired once the SRV and
     * TXT details arrive. */
    ck_assert_uint_eq(globalInterceptedMdnsMessages, previousSendCount);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);

    UA_ByteString_clear(&ptrPacket);
    UA_ByteString_clear(&announcePacket);
}
END_TEST

START_TEST(PublicApiInjectedPtrIgnoredWhenFollowedByGoodbye) {
    const char *serverName = "InjectedPtrRemoved";
    const char *hostname = "ptr-removed-host";
    char serviceInstance[128];
    char targetHost[64];
    UA_ByteString ptrPacket = UA_BYTESTRING_NULL;
    UA_ByteString srvPacket = UA_BYTESTRING_NULL;
    UA_ByteString goodbyePacket = UA_BYTESTRING_NULL;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedPtrPacket(serviceInstance, 600, &ptrPacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7778, NULL, NULL,
                            false, 0, true, 600, &srvPacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7778, NULL, NULL,
                            false, 0, true, 0, &goodbyePacket);

    size_t previousSendCount = globalInterceptedMdnsMessages;
    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &ptrPacket);
    injectMdnsPacket(testUdpCmLds, &srvPacket);
    injectMdnsPacket(testUdpCmLds, &goodbyePacket);
    iterateDiscoveryServers(2);

    /* The mdnsd driver is passive: it never issues a query in response to
     * an incoming PTR. The SRV record alone (without matching TXT) is not
     * sufficient to register the server, and the goodbye packet does not
     * remove anything because no entry was ever registered. */
    ck_assert_uint_eq(globalInterceptedMdnsMessages, previousSendCount);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkRemoveCalls, 0);

    UA_ByteString_clear(&ptrPacket);
    UA_ByteString_clear(&srvPacket);
    UA_ByteString_clear(&goodbyePacket);
}
END_TEST

START_TEST(PublicApiSelfAnnounceIsIgnored) {
    char selfServiceInstance[128];
    UA_ByteString selfPacket = UA_BYTESTRING_NULL;
    TestUdpIntercept *intercepts[] = {testUdpInterceptLds, testUdpInterceptRegister};
    MdnsMessageExpectation selfExpectation =
        {"LDS_public_api", "/", "NA", 4840, true, 0, false};
    size_t initialEntryCount = countServersOnNetwork(serverLds);

    waitForMdnsServiceInstanceName(intercepts, 2, 0, &selfExpectation,
                                   selfServiceInstance, sizeof(selfServiceInstance));

    buildInjectedMdnsPacket(selfServiceInstance, "self-loop.local.", 4840, "/", "NA",
                            true, 600, true, 600, &selfPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &selfPacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);
    ck_assert_uint_eq(countServersOnNetwork(serverLds), initialEntryCount);

    UA_ByteString_clear(&selfPacket);
}
END_TEST

START_TEST(PublicApiRepeatedAnnounceTriggersCallbackAgain) {
    const char *serverName = "InjectedRepeat";
    const char *hostname = "repeat-host";
    char serviceInstance[128];
    char expectedServerName[128];
    char targetHost[64];
    UA_ByteString announcePacket = UA_BYTESTRING_NULL;
    UA_ByteString repeatPacket = UA_BYTESTRING_NULL;

    createInjectedServiceInstanceName(serviceInstance, sizeof(serviceInstance),
                                      serverName, hostname);
    createInjectedServerOnNetworkName(expectedServerName, sizeof(expectedServerName),
                                      serverName, hostname);
    createInjectedTargetName(targetHost, sizeof(targetHost), hostname);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7778, "/repeat",
                            "SV", true, 600, true, 600, &announcePacket);
    buildInjectedMdnsPacket(serviceInstance, targetHost, 7778, "/ignored-update",
                            "DA", true, 600, false, 0, &repeatPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &announcePacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 1);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 1);
    ck_assert(discoveryCounters.lastIsTxtReceived);
    ck_assert(discoveryCounters.lastIsServerAnnounce);

    injectMdnsPacket(testUdpCmLds, &repeatPacket);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 2);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkAnnounceCalls, 2);
    ck_assert_uint_eq(discoveryCounters.serverOnNetworkRemoveCalls, 0);
    ck_assert(discoveryCounters.lastIsServerAnnounce);
    ck_assert(discoveryCounters.lastIsTxtReceived);
    ck_assert_str_eq(discoveryCounters.lastServerName, expectedServerName);
    ck_assert_str_eq(discoveryCounters.lastDiscoveryUrl,
                     "opc.tcp://repeat-host.local:7778/repeat/ignored-update");
    ck_assert_str_eq(discoveryCounters.lastCapabilities, "DA");

    UA_ByteString_clear(&announcePacket);
    UA_ByteString_clear(&repeatPacket);
}
END_TEST

START_TEST(PublicApiRegisterDeregisterServerOnNetworkTriggersMdnsSendPath) {
    TestUdpIntercept *intercepts[] = {testUdpIntercept};
    size_t initialSends = globalInterceptedMdnsMessages;
    MdnsMessageExpectation registerExpectation =
        {"Register_public_api", "/", "NA", 16664, true, 0, false};
    MdnsMessageExpectation deregisterExpectation =
        {"Register_public_api", "/", "NA", 16664, false, 0, true};
    UA_Boolean announce = true;
    UA_KeyValuePair announcePair;
    UA_KeyValueMap announceParams = {1, &announcePair};
    UA_ServerOnNetwork son;
    UA_ServerOnNetwork_init(&son);

    son.serverName = UA_String_fromChars("Register_public_api");
    son.discoveryUrl = UA_String_fromChars("opc.tcp://localhost:16664");
    son.serverCapabilitiesSize = 1;
    son.serverCapabilities =
        (UA_String*)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_ne(son.serverCapabilities, NULL);
    son.serverCapabilities[0] = UA_String_fromChars("NA");

    announcePair.key = UA_QUALIFIEDNAME(0, "announce");
    UA_Variant_setScalar(&announcePair.value, &announce,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);

    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son,
                                                        announceParams),
                      UA_STATUSCODE_GOOD);
    waitForMdnsMessageAndAssert(intercepts, 1, initialSends, &registerExpectation);

    size_t sendsAfterRegister = globalInterceptedMdnsMessages;
    ck_assert_uint_gt(sendsAfterRegister, initialSends);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "Register_public_api"), 1);

    ck_assert_uint_eq(UA_Server_deregisterServerOnNetwork(server, son.serverName),
                      UA_STATUSCODE_GOOD);
    waitForMdnsMessageAndAssert(intercepts, 1, sendsAfterRegister,
                                &deregisterExpectation);

    ck_assert_uint_gt(getMaxInterceptedMdnsSequenceFor(intercepts, 1),
                      sendsAfterRegister);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "Register_public_api"), 0);

    UA_ServerOnNetwork_clear(&son);
}
END_TEST

START_TEST(MdnsUpdateOnlineOfflineIsIdempotent) {
    ck_assert_ptr_ne(server->discoveryDriver, NULL);
    TestUdpIntercept *intercepts[] = {testUdpIntercept};
    size_t initialSendCount = globalInterceptedMdnsMessages;
    size_t initialEntries = countServersOnNetwork(server);
    MdnsMessageExpectation expectation =
        {"RemoteStableServer", "/", "NA", 16665, true, 0, false};

    updateMdnsForDiscoveryUrl(server, "RemoteStableServer",
                              "opc.tcp://localhost:16665", true);
    waitForMdnsMessageAndAssert(intercepts, 1, initialSendCount, &expectation);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "RemoteStableServer"), 1);
    ck_assert_uint_eq(countServersOnNetwork(server), initialEntries + 1);

    size_t sendsAfterFirstAdd = globalInterceptedMdnsMessages;
    updateMdnsForDiscoveryUrl(server, "RemoteStableServer",
                              "opc.tcp://localhost:16665", true);
    ck_assert_uint_eq(globalInterceptedMdnsMessages, sendsAfterFirstAdd);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "RemoteStableServer"), 1);
    ck_assert_uint_eq(countServersOnNetwork(server), initialEntries + 1);

    updateMdnsForDiscoveryUrl(server, "RemoteStableServer",
                              "opc.tcp://localhost:16665", false);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "RemoteStableServer"), 0);
    ck_assert_uint_eq(countServersOnNetwork(server), initialEntries);

    size_t sendsAfterFirstRemove = globalInterceptedMdnsMessages;
    updateMdnsForDiscoveryUrl(server, "RemoteStableServer",
                              "opc.tcp://localhost:16665", false);
    ck_assert_uint_eq(globalInterceptedMdnsMessages, sendsAfterFirstRemove);
    ck_assert_uint_eq(countServersOnNetworkByName(server, "RemoteStableServer"), 0);
    ck_assert_uint_eq(countServersOnNetwork(server), initialEntries);
}
END_TEST

START_TEST(PublicApiIgnoresInvalidReceiveRecords) {
    size_t initialEntries = countServersOnNetwork(serverLds);
    UA_ByteString wrongDomainPacket = UA_BYTESTRING_NULL;
    UA_ByteString wrongTypePacket = UA_BYTESTRING_NULL;
    UA_ByteString wrongClassPacket = UA_BYTESTRING_NULL;

    buildInjectedTxtPacketWithClass("ignored._http._tcp.local.", "/", "NA",
                                    QCLASS_IN, 600, &wrongDomainPacket);
    buildInjectedARecordPacket("IgnoredType-remote._opcua-tcp._tcp.local.", QCLASS_IN,
                               600, &wrongTypePacket);
    buildInjectedTxtPacketWithClass("IgnoredClass-remote._opcua-tcp._tcp.local.", "/",
                                    "NA", 3, 600, &wrongClassPacket);

    resetDiscoveryCounters();
    injectMdnsPacket(testUdpCmLds, &wrongDomainPacket);
    injectMdnsPacket(testUdpCmLds, &wrongTypePacket);
    injectMdnsPacket(testUdpCmLds, &wrongClassPacket);

    ck_assert_uint_eq(discoveryCounters.serverOnNetworkCalls, 0);
    ck_assert_uint_eq(countServersOnNetwork(serverLds), initialEntries);

    UA_ByteString_clear(&wrongDomainPacket);
    UA_ByteString_clear(&wrongTypePacket);
    UA_ByteString_clear(&wrongClassPacket);
}
END_TEST

#endif /* UA_ENABLE_DISCOVERY_MULTICAST_MDNSD */

static Suite *
testSuite_DiscoveryMdnsd(void) {
    Suite *s = suite_create(MDNS_DRIVER_SUITE_NAME);

    addDriverInterfaceTests(s);

#if defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD)
    TCase *tc = tcase_create("Send path scaffolding");
    tcase_add_unchecked_fixture(tc, setup_server, teardown_server);
    tcase_add_checked_fixture(tc, NULL, checkThreadCallbackFailures);
    tcase_add_test(tc, MdnsStartupTriggersSendPath);
    tcase_add_test(tc, MdnsShutdownSendsSelfGoodbyeAndDrainsQueue);
    tcase_add_test(tc, MdnsUpdateOnlineOfflineTriggersSendPath);
    tcase_add_test(tc, MdnsUpdateOnlineOfflineIsIdempotent);
    tcase_add_test(tc, PublicApiRegisterDeregisterServerOnNetworkTriggersMdnsSendPath);
    suite_add_tcase(s, tc);

    TCase *tc_query = tcase_create("Query behavior");
    tcase_add_checked_fixture(tc_query, NULL, checkThreadCallbackFailures);
    tcase_add_test(tc_query, MdnsQueryPresenceSendsStartupPtrQuery);
    tcase_add_test(tc_query, MdnsQueryDetailsSendsSrvTxtQueriesForPtr);
    tcase_add_test(tc_query, MdnsQueryDetailsSendsTxtQueryForSrvOnly);
    tcase_add_test(tc_query, MdnsQueryDetailsSendsSrvQueryForTxtOnly);
    suite_add_tcase(s, tc_query);

    TCase *tc_config = tcase_create("Driver config mirroring");
    /* DISABLED: hangs after returning. See git log.
    tcase_add_test(tc_config, MdnsDriverParamCopiessettingsOnStartup);
    */
    (void)tc_config;
    suite_add_tcase(s, tc_config);

    TCase *tc_public_api_core = tcase_create("Public discovery API core");
    tcase_add_test(tc_public_api_core,
                   PublicApiFindServersOnNetworkAppliesLimitAfterCapabilityFilter);
    suite_add_tcase(s, tc_public_api_core);

    TCase *tc_integration = tcase_create("Public discovery API integration");
    tcase_add_unchecked_fixture(tc_integration,
                                setup_public_api_servers,
                                teardown_public_api_servers);
    tcase_add_checked_fixture(tc_integration, NULL,
                              checkThreadCallbackFailures);
    tcase_add_test(tc_integration, PublicApiRegisterDeregisterCallback);
    tcase_add_test(tc_integration, PublicApiDeregisterDiscoveryKeepsLocalMdnsRecord);
    tcase_add_test(tc_integration, PublicApiFindServersOnNetworkListsRegisteredServers);
    tcase_add_test(tc_integration, PublicApiInjectedPtrIsIgnoredWithoutDetails);
    tcase_add_test(tc_integration,
                   PublicApiInjectedPtrDoesNotEmitQueryWhenDetailsArrive);
    tcase_add_test(tc_integration,
                   PublicApiInjectedPtrIgnoredWhenFollowedByGoodbye);
    tcase_add_test(tc_integration, PublicApiSelfAnnounceIsIgnored);
    tcase_add_test(tc_integration, PublicApiRepeatedAnnounceTriggersCallbackAgain);
    tcase_add_test(tc_integration,
                   PublicApiInjectedTxtThenSrvTriggersAnnounceCallback);
    tcase_add_test(tc_integration,
                   PublicApiInjectedSrvThenTxtUpdatesCallbackState);
    tcase_add_test(tc_integration, PublicApiInjectedTtlZeroRemovesRemoteServer);
    tcase_add_test(tc_integration, PublicApiInjectedRemoteServerIsNotMirroredBackOut);
    tcase_add_test(tc_integration, PublicApiIgnoresInvalidReceiveRecords);
    suite_add_tcase(s, tc_integration);
#endif

    return s;
}

int
main(void) {
    Suite *s = testSuite_DiscoveryMdnsd();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else

int
main(void) {
    return EXIT_SUCCESS;
}

#endif
