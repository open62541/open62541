/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * documentation not really included, placeholders
 * .. _sks_server_tutorial:
 * this is an example code of a sks server
 * it has basic access control based on username/passwork
 * and all method defined in specification
 *
 * It should work as follows:
 * 1. a client should call addSecurityGroup first and SKS will maintain a keyList for this securityGroup
 * 2. then a publisher/subscriber can call getSecurityKeys using an encrypted channel
 * 3. when removeSecurityGroup is called, the keyList will be cleared
 * 
 */

#include <open62541/client_highlevel.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_sks.h>

#include <open62541_queue.h>

#include <signal.h>
#include <stdlib.h>

#include "common.h"
#include <../build/src_generated/open62541/namespace0_generated.h>
#include <../build/src_generated/open62541/nodeids.h>

typedef struct {
    UA_NodeId nodeId;
    UA_Byte userAccessUse;
    UA_Byte userAccessModify;
} UA_NodeAccess;

typedef struct {
    UA_String userName;
    UA_String password;
    UA_NodeAccess *accessList;
    size_t accessListLength;
} UA_SKS_UsernamePasswordLogin;

static UA_SKS_UsernamePasswordLogin logins[2] = {
    {UA_STRING_STATIC("peter"), UA_STRING_STATIC("peter123"), NULL, 0},
    {UA_STRING_STATIC("paula"), UA_STRING_STATIC("paula123"), NULL, 0}};

static UA_StatusCode (*activateSessionBase)(
    UA_Server *server, UA_AccessControl *ac,
    const UA_EndpointDescription *endpointDescription,
    const UA_ByteString *secureChannelRemoteCertificate, const UA_NodeId *sessionId,
    const UA_ExtensionObject *userIdentityToken, void **sessionContext);

static UA_StatusCode
activateSession(UA_Server *server, UA_AccessControl *ac,
                const UA_EndpointDescription *endpointDescription,
                const UA_ByteString *secureChannelRemoteCertificate,
                const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken,
                void **sessionContext) {
    // use the original function for validation
    UA_StatusCode ret = activateSessionBase(server, ac, endpointDescription,
                                            secureChannelRemoteCertificate, sessionId,
                                            userIdentityToken, sessionContext);

    if(UA_STATUSCODE_GOOD != ret)
        return ret;

    if(UA_EXTENSIONOBJECT_DECODED != userIdentityToken->encoding) {
        // error: the userIdentityToken has not been decoded.
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    if(UA_TYPES_USERNAMEIDENTITYTOKEN !=
       userIdentityToken->content.decoded.type->typeIndex) {
        // for now, we only support user names here
        return UA_STATUSCODE_BADIDENTITYTOKENREJECTED;
    }

    UA_UserNameIdentityToken *token =
        (UA_UserNameIdentityToken *)userIdentityToken->content.decoded.data;

    *sessionContext = NULL;

    for(size_t i = 0; i < sizeof(logins) / sizeof(UA_SKS_UsernamePasswordLogin); ++i) {
        if(UA_String_equal(&logins[i].userName, &token->userName)) {
            *sessionContext = &logins[i];
            break;
        }
    }

    // this should not happen, because the default implemenation also checks the user name
    if(!sessionContext)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    return UA_STATUSCODE_GOOD;
}

static UA_Byte
getAccessLevelUse(UA_Server *server, const void *sessionContext,
                  const UA_NodeId *nodeId) {
    if(!nodeId)
        return 0xFF;

    const UA_SKS_UsernamePasswordLogin *login =
        (const UA_SKS_UsernamePasswordLogin *)sessionContext;

    for(size_t j = 0; j < login->accessListLength; ++j) {
        if(UA_NodeId_equal(&login->accessList[j].nodeId, nodeId))
            return login->accessList[j].userAccessUse;
    }

    return 0xFF;
}

static UA_Byte
getAccessLevelModify(UA_Server *server, const void *sessionContext,
                     const UA_NodeId *nodeId) {
    if(!nodeId)
        return 0xFF;

    const UA_SKS_UsernamePasswordLogin *login =
        (const UA_SKS_UsernamePasswordLogin *)sessionContext;

    for(size_t j = 0; j < login->accessListLength; ++j) {
        if(UA_NodeId_equal(&login->accessList[j].nodeId, nodeId))
            return login->accessList[j].userAccessModify;
    }

    return 0xFF;
}

static UA_Byte
getUserAccessLevel(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeId, void *nodeContext) {
    return getAccessLevelUse(server, sessionContext, nodeId);
}

static UA_Boolean
getUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *methodId, void *methodContext) {
    return getAccessLevelModify(server, sessionContext, methodId);
}

static UA_Boolean
getUserExecutableOnObject(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext) {
    return getAccessLevelModify(server, sessionContext, methodId);
}

UA_Boolean running = true;
static void
stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int
main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc < 3) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Missing arguments. Arguments are "
                     "<server-certificate.der> <private-key.der> "
                     "[<trustlist1.crl>, ...]");
        return EXIT_FAILURE;
    }

    UA_StatusCode retval;

    /* Load certificate and private key */
    UA_ByteString certificate = loadFile(argv[1]);
    UA_ByteString privateKey = loadFile(argv[2]);

    /* Load the trustlist */
    size_t trustListSize = 0;
    if(argc > 3)
        trustListSize = (size_t)argc - 3;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t i = 0; i < trustListSize; ++i)
        trustList[i] = loadFile(argv[i + 3]);

    /* Loading of a revocation list currently unsupported */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_Server *server = UA_Server_new();

    UA_ServerConfig *config = UA_Server_getConfig(server);

    retval = UA_ServerConfig_setDefaultWithSecurityPolicies(
        config, 4841, &certificate, &privateKey, trustList, trustListSize, NULL, 0,
        revocationList, revocationListSize);

    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_ServerConfig_addSecurityPolicy_Pubsub_Aes128ctr(config, &certificate,
                                                                &privateKey);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_ServerConfig_addSecurityPolicy_Pubsub_Aes256ctr(config, &certificate,
                                                                &privateKey);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    logins[1].accessListLength = 1;
    logins[1].accessList = (UA_NodeAccess *)malloc(sizeof(UA_NodeAccess) * 1);
    logins[1].accessList[0].nodeId = NODEID_SKS_AddSecurityGroup;
    logins[1].accessList[0].userAccessModify = false;
    logins[1].accessList[0].userAccessUse = false;

    UA_UsernamePasswordLogin
        defaultAccessControlLogins[sizeof(logins) / sizeof(UA_SKS_UsernamePasswordLogin)];

    for(size_t i = 0; i < sizeof(logins) / sizeof(UA_SKS_UsernamePasswordLogin); ++i) {
        defaultAccessControlLogins[i].username = logins[i].userName;
        defaultAccessControlLogins[i].password = logins[i].password;
    };

    /* Disable anonymous logins, enable two user/password logins */
    config->accessControl.clear(&config->accessControl);
    retval = UA_AccessControl_default(
        config, false,
        &config->securityPolicies[config->securityPoliciesSize - 1].policyUri, 2,
        defaultAccessControlLogins);

    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    activateSessionBase = config->accessControl.activateSession;
    config->accessControl.activateSession = activateSession;
    config->accessControl.getUserAccessLevel = getUserAccessLevel;
    config->accessControl.getUserExecutable = getUserExecutable;
    config->accessControl.getUserExecutableOnObject = getUserExecutableOnObject;

    retval = UA_Server_addSKS(server);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_Server_run(server, &running);

cleanup:
    UA_free(logins[1].accessList);
    UA_Server_delete(server);
    UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", UA_StatusCode_name(retval));
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
