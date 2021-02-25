/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2021 (c) basysKom GmbH <opensource@basyskom.com
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <check.h>
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

UA_LocalizedText displayNameDe;
UA_LocalizedText descriptionDe;

static UA_StatusCode readDisplayName(UA_Server *s, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeId,
                                     void *nodeContext, size_t localeIdsSize,
                                     const UA_LocaleId *localeIds, UA_LocalizedText *value) {
    (void)s;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;

    const UA_String de = UA_STRING("de");

    if (localeIdsSize && localeIds && UA_String_equal(localeIds, &de))
        UA_LocalizedText_copy(&displayNameDe, value);
    else
        *value = UA_LOCALIZEDTEXT_ALLOC("en", "MyDisplayName");

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readDescription(UA_Server *s, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeId,
                                     void *nodeContext, size_t localeIdsSize,
                                     const UA_LocaleId *localeIds, UA_LocalizedText *value) {
    (void)s;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;

    const UA_String de = UA_STRING("de");

    if (localeIdsSize && localeIds && UA_String_equal(localeIds, &de))
        UA_LocalizedText_copy(&descriptionDe, value);
    else
        *value = UA_LOCALIZEDTEXT_ALLOC("en", "MyDescription");

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeDisplayName(UA_Server *s, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeId,
                                     void *nodeContext, const UA_LocalizedText *value) {
    (void)s;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;

    const UA_String de = UA_STRING("de");

    if (UA_String_equal(&value->locale, &de)) {
        UA_LocalizedText_clear(&displayNameDe);
        UA_LocalizedText_copy(value, &displayNameDe);
        return UA_STATUSCODE_GOOD;
    }


    return UA_STATUSCODE_BADWRITENOTSUPPORTED;
}

static UA_StatusCode writeDescription(UA_Server *s, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeId,
                                     void *nodeContext, const UA_LocalizedText *value) {
    (void)s;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;

    const UA_String de = UA_STRING("de");

    if (UA_String_equal(&value->locale, &de)) {
        UA_LocalizedText_clear(&descriptionDe);
        UA_LocalizedText_copy(value, &descriptionDe);
        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADWRITENOTSUPPORTED;
}

static void
addTestVariable(void) {
    UA_DateTime now = 0;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
    UA_Variant_setScalar(&attr.value, &now, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "test-variable");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "test-variable");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_StatusCode retval = UA_Server_addVariableNode(server, currentNodeId, parentNodeId,
                                                     parentReferenceNodeId, currentName,
                                                     variableTypeNodeId, attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void
addLocalizedAttributeSourceToTestVariable(void) {
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "test-variable");
    UA_LocalizedAttributeSource source;
    source.readDisplayName = readDisplayName;
    source.writeDisplayName = writeDisplayName;
    source.readDescription = readDescription;
    source.writeDescription = writeDescription;
    UA_StatusCode retval = UA_Server_setNodeLocalizedAttributeSource(server, currentNodeId, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

THREAD_CALLBACK(serverloop) {
#ifndef WIN32
    (void)_;
#else
    (void)lpParam;
#endif

    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}
static void setup(void) {
    displayNameDe = UA_LOCALIZEDTEXT_ALLOC("de", "MeinDisplayName");
    descriptionDe = UA_LOCALIZEDTEXT_ALLOC("de", "MeineBeschreibung");

    running = true;
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_run_startup(server);
    addTestVariable();
    addLocalizedAttributeSourceToTestVariable();
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    UA_LocalizedText_clear(&displayNameDe);
    UA_LocalizedText_clear(&descriptionDe);
}

START_TEST(client_readDisplayNameAttribute) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        conf->sessionLocaleIdsSize = 1;
        conf->sessionLocaleIds = UA_LocaleId_new();
        *conf->sessionLocaleIds = UA_STRING_ALLOC("de");

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");
        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDisplayNameAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("de");
        const UA_String expectedText = UA_STRING("MeinDisplayName");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

START_TEST(client_readDisplayNameAttributeNoLocale) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");
        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDisplayNameAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("en");
        const UA_String expectedText = UA_STRING("MyDisplayName");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

START_TEST(client_readDescriptionAttribute) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        conf->sessionLocaleIdsSize = 1;
        conf->sessionLocaleIds = UA_LocaleId_new();
        *conf->sessionLocaleIds = UA_STRING_ALLOC("de");

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");
        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDescriptionAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("de");
        const UA_String expectedText = UA_STRING("MeineBeschreibung");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

START_TEST(client_readDescriptionAttributeNoLocale) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");
        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDescriptionAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("en");
        const UA_String expectedText = UA_STRING("MyDescription");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

START_TEST(client_writeDisplayNameAttribute) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        conf->sessionLocaleIdsSize = 1;
        conf->sessionLocaleIds = UA_LocaleId_new();
        *conf->sessionLocaleIds = UA_STRING_ALLOC("de");

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");

        UA_LocalizedText updateValue = UA_LOCALIZEDTEXT("de", "MeinDisplayName2");

        retval = UA_Client_writeDisplayNameAttribute(client, nodeId, &updateValue);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDisplayNameAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("de");
        const UA_String expectedText = UA_STRING("MeinDisplayName2");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

START_TEST(client_writeDescriptionAttribute) {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig *conf = UA_Client_getConfig(client);
        UA_ClientConfig_setDefault(conf);

        conf->sessionLocaleIdsSize = 1;
        conf->sessionLocaleIds = UA_LocaleId_new();
        *conf->sessionLocaleIds = UA_STRING_ALLOC("de");

        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_NodeId nodeId = UA_NODEID_STRING(1, "test-variable");

        UA_LocalizedText updateValue = UA_LOCALIZEDTEXT("de", "MeineBeschreibung2");

        retval = UA_Client_writeDisplayNameAttribute(client, nodeId, &updateValue);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_LocalizedText result;
        UA_LocalizedText_init(&result);

        retval = UA_Client_readDisplayNameAttribute(client, nodeId, &result);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        const UA_String expectedLocale = UA_STRING("de");
        const UA_String expectedText = UA_STRING("MeineBeschreibung2");

        ck_assert(UA_String_equal(&expectedLocale, &result.locale));
        ck_assert(UA_String_equal(&expectedText, &result.text));

        UA_LocalizedText_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

static Suite* testSuite_localization(void) {
    Suite *s = suite_create("Server localization");
    TCase *localizationCallback = tcase_create("server localization");

    tcase_add_checked_fixture(localizationCallback, setup, teardown);
    tcase_add_test(localizationCallback, client_readDisplayNameAttribute);
    tcase_add_test(localizationCallback, client_readDisplayNameAttributeNoLocale);
    tcase_add_test(localizationCallback, client_readDescriptionAttribute);
    tcase_add_test(localizationCallback, client_readDescriptionAttributeNoLocale);
    tcase_add_test(localizationCallback, client_writeDisplayNameAttribute);
    tcase_add_test(localizationCallback, client_writeDescriptionAttribute);

    suite_add_tcase(s, localizationCallback);
    return s;
}

int main(void) {
    Suite *s = testSuite_localization();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
