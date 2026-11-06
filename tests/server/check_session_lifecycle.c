/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* OPC 10000-4 v1.05.07, 5.7.2.1 (CreateSession, activation, lifetime):
 * https://reference.opcfoundation.org/specs/OPC-10000-4/5.7.2.1
 * Expectations come from a separately checked, LLM-assisted Lean model.
 * See session_lifecycle/README.md for interpretation choices and boundaries.
 * Native checks use production allocation/activation/close/cleanup functions;
 * TCP replays also exercise CreateSession and token usability on the wire.
 * No test-side eviction selector or direct activation-state assignment. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "session_lifecycle_expectations.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
#if UA_MULTITHREADING >= 100
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE serverThread;
#endif

static const SessionModelStep *
modelStep(const SessionModelCase *history, unsigned step) {
    return &sessionModelSteps[history->steps[step]];
}

static void
quietLog(void *context, UA_LogLevel level, UA_LogCategory category, const char *message,
         va_list args) {
    (void)context;
    (void)level;
    (void)category;
    (void)message;
    (void)args;
}

#if UA_MULTITHREADING >= 100
THREAD_CALLBACK(serverLoop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, false);
    return 0;
}
#endif

static void
startServer(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->logging->log = quietLog;
    cfg->maxSecureChannels = 32;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void
stopServer(void) {
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    server = NULL;
}

static void
clearSessions(void) {
    UA_LOCK(&server->serviceMutex);
    while(!LIST_EMPTY(&server->sessions))
        UA_Session_remove(server, &LIST_FIRST(&server->sessions)->session,
                          UA_SHUTDOWNREASON_CLOSE);
    UA_UNLOCK(&server->serviceMutex);
    UA_Server_run_iterate(server, false);
}

static void
observeStep(const char *mode, unsigned caseIndex, unsigned stepIndex,
            UA_StatusCode status, const UA_NodeId *tokens) {
    unsigned live = 0, activeMask = 0;
    UA_LOCK(&server->serviceMutex);
    for(unsigned id = 0; id < SESSION_MODEL_HANDLES; id++) {
        UA_Session *s = findSessionByToken(server, &tokens[id]);
        if(s) {
            live |= 1u << id;
            if(s->state == UA_SESSIONSTATE_ACTIVATED)
                activeMask |= 1u << id;
        }
    }
    unsigned count = (unsigned)server->sessionCount;
    unsigned activeCount = (unsigned)server->activeSessionCount;
    UA_UNLOCK(&server->serviceMutex);
    const SessionModelCase *history = &sessionModelCases[caseIndex];
    const SessionModelStep *expected = modelStep(history, stepIndex);
    printf("MODEL_EVENT {\"mode\":\"%s\",\"case\":%u,\"step\":%u,\"status\":%u,"
           "\"kind\":%u,\"arg\":%u,"
           "\"liveMask\":%u,\"activeMask\":%u,\"count\":%u,\"activeCount\":%u}\n",
           mode, caseIndex, stepIndex, (unsigned)status, expected->kind, expected->arg,
           live, activeMask, count, activeCount);
    fflush(stdout);
    ck_assert_msg(status == expected->status && live == expected->liveMask &&
                      activeMask == expected->activeMask && count == expected->count &&
                      activeCount == expected->activeCount,
                  "%s case=%u step=%u: status=%u live=%u active=%u count=%u/%u",
                  history->name, caseIndex, stepIndex, (unsigned)status, live, activeMask,
                  count, activeCount);
}

static void
initTokens(UA_NodeId *tokens) {
    for(unsigned id = 0; id < SESSION_MODEL_HANDLES; id++)
        tokens[id] = UA_NODEID_NUMERIC(1, 1000 + id);
}

static UA_StatusCode
nativeOperation(const SessionModelStep *op, unsigned *next, UA_SecureChannel *channel,
                UA_NodeId *tokens) {
    UA_StatusCode status = UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    switch(op->kind) {
        case 0: {
            ck_assert(*next < SESSION_MODEL_HANDLES);
            UA_CreateSessionRequest request;
            UA_CreateSessionRequest_init(&request);
            request.requestedSessionTimeout = (UA_Double)op->arg;
            UA_Session *s = NULL;
            status = UA_Session_create(server, channel, &request, &s);
            if(status == UA_STATUSCODE_GOOD) {
                UA_NodeId_clear(&tokens[*next]);
                ck_assert_uint_eq(UA_NodeId_copy(&s->authenticationToken, &tokens[*next]),
                                  UA_STATUSCODE_GOOD);
            }
            (*next)++;
            break;
        }
        case 1: {
            UA_ActivateSessionRequest request;
            UA_ActivateSessionRequest_init(&request);
            request.requestHeader.authenticationToken = tokens[op->arg];
            UA_ActivateSessionResponse response;
            UA_ActivateSessionResponse_init(&response);
            Service_ActivateSession(server, channel, &request, &response);
            status = response.responseHeader.serviceResult;
            UA_ActivateSessionResponse_clear(&response);
            break;
        }
        case 2: {
            UA_CloseSessionRequest request;
            UA_CloseSessionRequest_init(&request);
            request.requestHeader.authenticationToken = tokens[op->arg];
            UA_CloseSessionResponse response;
            UA_CloseSessionResponse_init(&response);
            Service_CloseSession(server, channel, &request, &response);
            status = response.responseHeader.serviceResult;
            UA_CloseSessionResponse_clear(&response);
            break;
        }
        case 3:
            UA_fakeSleep(op->arg);
            cleanupSessions(server, server->config.eventLoop->dateTime_nowMonotonic(
                                        server->config.eventLoop));
            break;
        default:
            ck_abort_msg("Unknown generated operation");
    }
    UA_UNLOCK(&server->serviceMutex);
    return status;
}

START_TEST(Session_model_native) {
    startServer();
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.state = UA_SECURECHANNELSTATE_OPEN;
    channel.securityMode = UA_MESSAGESECURITYMODE_NONE;
    channel.securityPolicy = &server->config.securityPolicies[0];
    ck_assert_uint_eq(channel.securityPolicy->policyType, UA_SECURITYPOLICYTYPE_NONE);
    channel.securityToken.channelId = 42;
    const char *selected = getenv("SESSION_MODEL_CASE");
    unsigned executed = 0;
    for(unsigned c = 0; c < SESSION_MODEL_CASES; c++) {
        if(selected && strcmp(selected, sessionModelCases[c].name) != 0)
            continue;
        const SessionModelCase *history = &sessionModelCases[c];
        executed++;
        server->config.maxSessions = history->maximum;
        UA_NodeId tokens[SESSION_MODEL_HANDLES];
        initTokens(tokens);
        unsigned next = 0;
        for(unsigned step = 0; step < history->size; step++) {
            UA_StatusCode status =
                nativeOperation(modelStep(history, step), &next, &channel, tokens);
            observeStep("native", c, step, status, tokens);
        }
        clearSessions();
        for(unsigned id = 0; id < SESSION_MODEL_HANDLES; id++)
            UA_NodeId_clear(&tokens[id]);
    }
    ck_assert_msg(executed > 0, "No matching lifecycle trace");
    UA_SecureChannel_clear(&channel);
    stopServer();
}
END_TEST

#if UA_MULTITHREADING >= 100
static void
setToken(UA_Client *client, const UA_NodeId *token) {
    UA_NodeId_clear(&client->authenticationToken);
    ck_assert_uint_eq(UA_NodeId_copy(token, &client->authenticationToken),
                      UA_STATUSCODE_GOOD);
}

static UA_StatusCode
tcpActivate(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    UA_ActivateSessionResponse response;
    UA_ActivateSessionResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    UA_StatusCode status = response.responseHeader.serviceResult;
    UA_ActivateSessionResponse_clear(&response);
    return status;
}

static UA_StatusCode
tcpRead(UA_Client *client, const UA_NodeId *token) {
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.requestHeader.authenticationToken = *token;
    UA_ReadValueId node;
    UA_ReadValueId_init(&node);
    node.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    node.attributeId = UA_ATTRIBUTEID_VALUE;
    request.nodesToRead = &node;
    request.nodesToReadSize = 1;
    UA_ReadResponse response;
    UA_ReadResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_READREQUEST], &response,
                        &UA_TYPES[UA_TYPES_READRESPONSE]);
    UA_StatusCode status = response.responseHeader.serviceResult;
    if(status == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(response.resultsSize, 1);
        ck_assert_uint_eq(response.results[0].status, UA_STATUSCODE_GOOD);
        ck_assert(response.results[0].hasValue);
    }
    UA_ReadResponse_clear(&response);
    return status;
}

static void
tokenProbe(unsigned c, unsigned id, const char *op, UA_StatusCode status,
           UA_StatusCode expected) {
    printf("MODEL_PROBE {\"case\":%u,\"id\":%u,\"op\":\"%s\",\"status\":%u}\n", c, id, op,
           (unsigned)status);
    fflush(stdout);
    ck_assert_uint_eq(status, expected);
}

START_TEST(Session_model_tcp) {
    startServer();
    UA_atomic_store(&running, true);
    THREAD_CREATE(serverThread, serverLoop);
    for(unsigned c = 0; c < SESSION_MODEL_CASES; c++) {
        const SessionModelCase *history = &sessionModelCases[c];
        if(!history->tcp)
            continue;
        UA_LOCK(&server->serviceMutex);
        server->config.maxSessions = history->maximum;
        UA_UNLOCK(&server->serviceMutex);
        UA_Client *clients[SESSION_MODEL_HANDLES];
        UA_NodeId tokens[SESSION_MODEL_HANDLES];
        initTokens(tokens);
        unsigned next = 0;
        for(unsigned id = 0; id < SESSION_MODEL_HANDLES; id++) {
            clients[id] = UA_Client_newForUnitTest();
            ck_assert_ptr_nonnull(clients[id]);
            UA_Client_getConfig(clients[id])->logging->log = quietLog;
            ck_assert_uint_eq(
                UA_Client_connectSecureChannel(clients[id], "opc.tcp://localhost:4840"),
                UA_STATUSCODE_GOOD);
            ck_assert(UA_Client_getConfig(clients[id])->noSession);
        }
        for(unsigned step = 0; step < history->size; step++) {
            const SessionModelStep *op = modelStep(history, step);
            UA_StatusCode status = UA_STATUSCODE_GOOD;
            if(op->kind == 0) {
                UA_CreateSessionRequest request;
                UA_CreateSessionRequest_init(&request);
                request.requestedSessionTimeout = (UA_Double)op->arg;
                UA_CreateSessionResponse response;
                UA_CreateSessionResponse_init(&response);
                __UA_Client_Service(clients[next], &request,
                                    &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST], &response,
                                    &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
                status = response.responseHeader.serviceResult;
                if(status == UA_STATUSCODE_GOOD) {
                    ck_assert_double_eq(response.revisedSessionTimeout,
                                        (UA_Double)op->arg);
                    UA_NodeId_clear(&tokens[next]);
                    ck_assert_uint_eq(
                        UA_NodeId_copy(&response.authenticationToken, &tokens[next]),
                        UA_STATUSCODE_GOOD);
                }
                UA_CreateSessionResponse_clear(&response);
                next++;
            } else if(op->kind == 1) {
                setToken(clients[op->arg], &tokens[op->arg]);
                status = tcpActivate(clients[op->arg]);
            } else if(op->kind == 2) {
                setToken(clients[op->arg], &tokens[op->arg]);
                UA_CloseSessionRequest request;
                UA_CloseSessionRequest_init(&request);
                UA_CloseSessionResponse response;
                UA_CloseSessionResponse_init(&response);
                __UA_Client_Service(clients[op->arg], &request,
                                    &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST], &response,
                                    &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
                status = response.responseHeader.serviceResult;
                UA_CloseSessionResponse_clear(&response);
            } else {
                ck_abort_msg("TCP corpus excludes clock advances");
            }
            observeStep("tcp", c, step, status, tokens);
        }
        const SessionModelStep *final = modelStep(history, history->size - 1);
        for(unsigned id = 0; id < next; id++) {
            UA_StatusCode expected = final->liveMask & (1u << id)
                                         ? UA_STATUSCODE_GOOD
                                         : UA_STATUSCODE_BADSESSIONIDINVALID;
            setToken(clients[id], &tokens[id]);
            if(!(final->activeMask & (1u << id)))
                tokenProbe(c, id, "activate", tcpActivate(clients[id]), expected);
            tokenProbe(c, id, "read", tcpRead(clients[id], &tokens[id]), expected);
        }
        for(unsigned id = 0; id < SESSION_MODEL_HANDLES; id++) {
            UA_Client_disconnect(clients[id]);
            UA_Client_delete(clients[id]);
            UA_NodeId_clear(&tokens[id]);
        }
        UA_LOCK(&server->serviceMutex);
        while(!LIST_EMPTY(&server->sessions))
            UA_Session_remove(server, &LIST_FIRST(&server->sessions)->session,
                              UA_SHUTDOWNREASON_CLOSE);
        UA_UNLOCK(&server->serviceMutex);
    }
    UA_atomic_store(&running, false);
    THREAD_JOIN(serverThread);
    stopServer();
}
END_TEST
#endif

int
main(void) {
    Suite *suite = suite_create("Proof-backed Session lifecycle");
    TCase *tc = tcase_create("model corpus");
    tcase_set_timeout(tc, 180);
    tcase_add_test(tc, Session_model_native);
#if UA_MULTITHREADING >= 100
    if(!getenv("SESSION_MODEL_NATIVE_ONLY"))
        tcase_add_test(tc, Session_model_tcp);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
