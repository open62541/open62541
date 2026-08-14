/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* OPC UA HTTPS interoperability client.
 *
 * Usage:
 *   check_interop_client_https <opc.https-url> <client-cert.der>
 *       <client-key.der> <server-tls-cert.der>
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t notifications;
    UA_DateTime lastValue;
} SubscriptionResult;

static UA_ByteString
loadFile(const char *path) {
    UA_ByteString result = UA_BYTESTRING_NULL;
    FILE *fp = fopen(path, "rb");
    if(!fp)
        return result;
    if(fseek(fp, 0, SEEK_END) != 0)
        goto cleanup;
    long length = ftell(fp);
    if(length <= 0 || fseek(fp, 0, SEEK_SET) != 0)
        goto cleanup;
    if(UA_ByteString_allocBuffer(&result, (size_t)length) != UA_STATUSCODE_GOOD)
        goto cleanup;
    if(fread(result.data, 1, result.length, fp) != result.length)
        UA_ByteString_clear(&result);
cleanup:
    fclose(fp);
    return result;
}

static void
dataChangeCallback(UA_Client *client, UA_UInt32 subscriptionId,
                   void *subscriptionContext, UA_UInt32 monitoredItemId,
                   void *monitoredItemContext, UA_DataValue *value) {
    (void)client;
    (void)subscriptionId;
    (void)subscriptionContext;
    (void)monitoredItemId;
    SubscriptionResult *result = (SubscriptionResult *)monitoredItemContext;
    if(!value->hasValue ||
       !UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME]))
        return;
    result->lastValue = *(UA_DateTime *)value->value.data;
    result->notifications++;
}

static UA_StatusCode
configureClient(UA_ClientConfig *config, const UA_ByteString *clientCertificate,
                const UA_ByteString *clientPrivateKey,
                const UA_ByteString *serverCertificate) {
    UA_StatusCode res = UA_ClientConfig_setDefaultEncryption(
        config, *clientCertificate, *clientPrivateKey,
        serverCertificate, 1, NULL, 0);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    *config->logging = UA_Log_Stdout_withLevel(UA_LOGLEVEL_WARNING);
    UA_String_clear(&config->clientDescription.applicationUri);
    config->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.client.application");
    if(config->clientDescription.applicationUri.length == 0)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    res = UA_ByteString_copy(serverCertificate, &config->httpCaCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_ByteString_copy(clientCertificate,
                             &config->httpClientCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_ByteString_copy(clientPrivateKey,
                             &config->httpClientPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* Transport-profile matching keeps endpoint selection on HTTPS even when
     * the server also advertises stronger TCP endpoints. Let the endpoint
     * choose its OPC UA SecurityPolicy and UserTokenPolicy. */
    config->timeout = 15000;
    config->httpTimeout = 15;
    config->outStandingPublishRequests = 3;
    return UA_STATUSCODE_GOOD;
}

static int
runSubscription(UA_Client *client, const char *identityLabel) {
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedPublishingInterval = 200.0;
    request.requestedMaxKeepAliveCount = 2;
    request.requestedLifetimeCount = 30;
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "[https-interop] %s CreateSubscription failed: %s\n",
                identityLabel,
                UA_StatusCode_name(response.responseHeader.serviceResult));
        UA_CreateSubscriptionResponse_clear(&response);
        return EXIT_FAILURE;
    }

    SubscriptionResult result = {0};
    UA_MonitoredItemCreateRequest item =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    item.requestedParameters.samplingInterval = 100.0;
    item.requestedParameters.queueSize = 10;
    UA_MonitoredItemCreateResult itemResult =
        UA_Client_MonitoredItems_createDataChange(
            client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,
            item, &result, dataChangeCallback, NULL);
    if(itemResult.statusCode != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "[https-interop] %s CreateMonitoredItems failed: %s\n",
                identityLabel, UA_StatusCode_name(itemResult.statusCode));
        UA_MonitoredItemCreateResult_clear(&itemResult);
        UA_CreateSubscriptionResponse_clear(&response);
        return EXIT_FAILURE;
    }
    UA_MonitoredItemCreateResult_clear(&itemResult);

    /* CurrentTime changes once per second. Three notifications require the
     * client to replenish its long-running Publish requests after responses. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < 150 && result.notifications < 3; i++) {
        res = UA_Client_run_iterate(client, 100);
        if(res != UA_STATUSCODE_GOOD)
            break;
    }

    UA_StatusCode deleteRes = UA_Client_Subscriptions_deleteSingle(
        client, response.subscriptionId);
    UA_CreateSubscriptionResponse_clear(&response);
    if(res != UA_STATUSCODE_GOOD || deleteRes != UA_STATUSCODE_GOOD ||
       result.notifications < 3) {
        fprintf(stderr,
                "[https-interop] %s subscription failed: iterate=%s, "
                "delete=%s, notifications=%zu\n",
                identityLabel, UA_StatusCode_name(res),
                UA_StatusCode_name(deleteRes), result.notifications);
        return EXIT_FAILURE;
    }

    printf("[https-interop] PASS: %s subscription delivered %zu values\n",
           identityLabel, result.notifications);
    return EXIT_SUCCESS;
}

static int
runIdentityCase(const char *url, UA_Boolean username,
                const UA_ByteString *clientCertificate,
                const UA_ByteString *clientPrivateKey,
                const UA_ByteString *serverCertificate) {
    const char *label = username ? "username" : "anonymous";
    UA_Client *client = UA_Client_new();
    if(!client)
        return EXIT_FAILURE;
    UA_StatusCode res = configureClient(UA_Client_getConfig(client),
                                        clientCertificate, clientPrivateKey,
                                        serverCertificate);
    if(res == UA_STATUSCODE_GOOD) {
        if(username)
            res = UA_Client_connectUsername(client, url, "user1", "password");
        else
            res = UA_Client_connect(client, url);
    }
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "[https-interop] %s connect failed: %s\n",
                label, UA_StatusCode_name(res));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Client_readValueAttribute(
        client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
        &value);
    UA_Boolean valid = res == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_clear(&value);
    if(!valid) {
        fprintf(stderr, "[https-interop] %s CurrentTime read failed: %s\n",
                label, UA_StatusCode_name(res));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    printf("[https-interop] PASS: %s session and read\n", label);

    int result = runSubscription(client, label);
    res = UA_Client_disconnect(client);
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "[https-interop] %s disconnect failed: %s\n",
                label, UA_StatusCode_name(res));
        result = EXIT_FAILURE;
    }
    UA_Client_delete(client);
    return result;
}

static int
runWrongPasswordCase(const char *url,
                     const UA_ByteString *clientCertificate,
                     const UA_ByteString *clientPrivateKey,
                     const UA_ByteString *serverCertificate) {
    UA_Client *client = UA_Client_new();
    if(!client)
        return EXIT_FAILURE;
    UA_StatusCode res = configureClient(UA_Client_getConfig(client),
                                        clientCertificate, clientPrivateKey,
                                        serverCertificate);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Client_connectUsername(client, url, "user1", "wrong-password");
    UA_Client_delete(client);
    if(res != UA_STATUSCODE_BADUSERACCESSDENIED &&
       res != UA_STATUSCODE_BADIDENTITYTOKENREJECTED &&
       res != UA_STATUSCODE_BADIDENTITYTOKENINVALID) {
        fprintf(stderr,
                "[https-interop] wrong password did not produce an "
                "authentication rejection: %s\n", UA_StatusCode_name(res));
        return EXIT_FAILURE;
    }
    printf("[https-interop] PASS: wrong password rejected (%s)\n",
           UA_StatusCode_name(res));
    return EXIT_SUCCESS;
}

int
main(int argc, char **argv) {
    if(argc != 5 || strncmp(argv[1], "opc.https://", 12) != 0) {
        fprintf(stderr, "Usage: %s <opc.https-url> <client-cert.der> "
                "<client-key.der> <server-tls-cert.der>\n", argv[0]);
        return EXIT_FAILURE;
    }
    UA_ByteString clientCertificate = loadFile(argv[2]);
    UA_ByteString clientPrivateKey = loadFile(argv[3]);
    UA_ByteString serverCertificate = loadFile(argv[4]);
    if(clientCertificate.length == 0 || clientPrivateKey.length == 0 ||
       serverCertificate.length == 0) {
        fprintf(stderr, "[https-interop] failed to load certificates\n");
        UA_ByteString_clear(&clientCertificate);
        UA_ByteString_clear(&clientPrivateKey);
        UA_ByteString_clear(&serverCertificate);
        return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;
    result |= runIdentityCase(argv[1], false, &clientCertificate,
                              &clientPrivateKey, &serverCertificate);
    result |= runIdentityCase(argv[1], true, &clientCertificate,
                              &clientPrivateKey, &serverCertificate);
    result |= runWrongPasswordCase(argv[1], &clientCertificate,
                                   &clientPrivateKey, &serverCertificate);

    UA_ByteString_clear(&clientCertificate);
    UA_ByteString_clear(&clientPrivateKey);
    UA_ByteString_clear(&serverCertificate);
    return result == EXIT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
