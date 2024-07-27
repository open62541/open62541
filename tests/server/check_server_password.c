/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/plugin/certificategroup.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

#if defined(__OpenBSD__) || defined(__linux__)
#include <pwd.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <check.h>

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

#if defined(__OpenBSD__)
static UA_StatusCode
loginCallback(const UA_String *userName, const UA_ByteString *password,
              size_t loginSize, const UA_UsernamePasswordLogin *loginList,
              void **sessionContext, void *loginContext) {
    char *pass;
    size_t i;
    int userok = 0, passok = 0;

    /* UA_ByteString has no terminating NUL byte */
    pass = UA_malloc(password->length + 1);
    if (pass == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(pass, password->data, password->length);
    pass[password->length] = '\0';

    /* Always run through full loop to avoid timing attack. */
    for (i = 0; i < loginSize; i++, loginList++) {
        char hash[_PASSWORD_LEN + 1];
        size_t hashlen;

        if (userName->length == loginList->username.length &&
            timingsafe_bcmp(userName->data, loginList->username.data,
            userName->length) == 0)
                userok = 1;
        else
                continue;

        /* UA_String has no terminating NUL byte */
        hashlen = loginList->password.length < _PASSWORD_LEN ?
            loginList->password.length : _PASSWORD_LEN;
        memcpy(hash, loginList->password.data, hashlen);
        hash[hashlen] = '\0';

        if (crypt_checkpass(pass, hash) == 0)
                passok = 1;
    }
    /* Do some work if user does not match to avoid user guessing. */
    if (!userok)
        crypt_checkpass(pass, NULL);

    UA_free(pass);
    return passok ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUSERACCESSDENIED;
}
#endif

#if defined(__linux__)
static UA_StatusCode
loginCallback(const UA_String *userName, const UA_ByteString *password,
    size_t loginSize, const UA_UsernamePasswordLogin *loginList,
    void **sessionContext, void *loginContext)
{
    const char *id = (char *)loginContext;
    char *pass;
    size_t i;
    int userok = 0, passok = 0;

    /* UA_ByteString has no terminating NUL byte */
    pass = (char *)UA_malloc(password->length + 1);
    if (pass == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(pass, password->data, password->length);
    pass[password->length] = '\0';

    /* Always run through full loop to avoid timing attack. */
    for (i = 0; i < loginSize; i++, loginList++) {
        const char *hash;
        char salt[1000];
        int dollar = 0;

        if (userName->length == loginList->username.length &&
            memcmp(userName->data, loginList->username.data,
            userName->length) == 0)
                userok = 1;
        else
                continue;

        /* Check if algorithm has required strength. */
        if (memcmp(id, loginList->password.data, strlen(id)) != 0)
                continue;

        for (i = 0; i < sizeof(salt) - 1 && i < loginList->password.length; i++) {
            if (dollar == 3)
                break;
            salt[i] = loginList->password.data[i];
            if (salt[i] == '$')
                dollar++;
        }
        salt[i] = '\0';
        hash = crypt(pass, salt);
        ck_assert_msg(hash, "crypt");
        if (memcmp(hash, loginList->password.data,
            loginList->password.length) == 0)
                passok = 1;
    }
    /* Do some work if user does not match to avoid user guessing. */
    if (!userok)
        crypt(pass, id);

    UA_free(pass);
    return passok ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUSERACCESSDENIED;
}
#endif

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert_msg(server, "UA_Server_new");
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String policy = UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_UsernamePasswordLogin login[] = {
        { UA_STRING_STATIC("user"),
#if defined(__OpenBSD__)
          UA_STRING_STATIC("$2b$08$nz828OX4t7a6Sg8JO/0GnO/bcfY0UyBmlAwkvIGE9ZaBq.0n2tkoS"),
#elif defined(__linux__)
          UA_STRING_STATIC("$6$uogBj0wZGItfBChT$jp2zOMGXvC0Jr2GxbYcAcuw2eBpuveezwuOB5EjX/QAXurprMGoG8W7/WKiic0utw0xnJ16tjtFS2UtDmeoYj0"),
#else
          UA_STRING_STATIC("pass"),
#endif
        },
    };
#if defined(__OpenBSD__) || defined(__linux__)
    UA_AccessControl_defaultWithLoginCallback(config, false, &policy,
        sizeof(login) / sizeof(login[0]), login, loginCallback, "$6$");
#else
    UA_AccessControl_default(config, false, &policy,
        sizeof(login) / sizeof(login[0]), login);
#endif
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
ClientConfig_setUsernamePassword(UA_ClientConfig *config,
    const UA_String *userName, const UA_String *password)
{
    UA_UserNameIdentityToken *identityToken;

    UA_ExtensionObject_clear(&config->userIdentityToken);
    UA_UserTokenPolicy_clear(&config->userTokenPolicy);
    UA_EndpointDescription_clear(&config->endpoint);

    identityToken = UA_UserNameIdentityToken_new();
    ck_assert_msg(identityToken, "UA_UserNameIdentityToken_new");
    config->userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    config->userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
    config->userIdentityToken.content.decoded.data = identityToken;

    UA_String_copy(userName, &identityToken->userName);
    UA_String_copy(password, &identityToken->password);
}

START_TEST(Password_good) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_msg(client, "UA_Client_new");
    UA_ClientConfig *config = UA_Client_getConfig(client);

    UA_String user = UA_STRING_STATIC("user");
    UA_String pass = UA_STRING_STATIC("pass");
    ClientConfig_setUsernamePassword(config, &user, &pass);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Password_bad) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_msg(client, "UA_Client_new");
    UA_ClientConfig *config = UA_Client_getConfig(client);

    UA_String user = UA_STRING_STATIC("user");
    UA_String pass = UA_STRING_STATIC("bad");
    ClientConfig_setUsernamePassword(config, &user, &pass);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Password_none) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_msg(client, "UA_Client_new");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADIDENTITYTOKENINVALID);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static Suite* testSuite_Password(void) {
    Suite *s = suite_create("Password");
    TCase *tc = tcase_create("Core");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Password_good);
    tcase_add_test(tc, Password_bad);
    tcase_add_test(tc, Password_none);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_Password();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
