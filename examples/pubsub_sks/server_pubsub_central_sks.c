/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdlib.h>

#include "common.h"

#define MAX_OPERATION_LIMIT 10000

#define MINUTE_SECONDS 60
#define MILLI_SECONDS 1000
#define MAX_OPERATION_LIMIT 10000

#define policUri "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"
#define DEMO_KEYLIFETIME_MINUTES 1
#define DEMO_MAXFUTUREKEYCOUNT 1
#define DEMO_MAXPASTKEYCOUNT 1
#define DEMO_SECURITYGROUPNAME "DemoSecurityGroup"

static void
disableAnonymous(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];

        for(size_t j = 0; j < ep->userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &ep->userIdentityTokens[j];
            if(utp->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                continue;

            UA_UserTokenPolicy_clear(utp);
            /* Move the last to this position */
            if(j + 1 < ep->userIdentityTokensSize) {
                ep->userIdentityTokens[j] =
                    ep->userIdentityTokens[ep->userIdentityTokensSize - 1];
                j--;
            }
            ep->userIdentityTokensSize--;
        }

        /* Delete the entire array if the last UserTokenPolicy was removed */
        if(ep->userIdentityTokensSize == 0) {
            UA_free(ep->userIdentityTokens);
            ep->userIdentityTokens = NULL;
        }
    }
}

#ifdef UA_ENABLE_ENCRYPTION
static void
disableUnencrypted(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        if(ep->securityMode != UA_MESSAGESECURITYMODE_NONE)
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize - 1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize == 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

static void
disableOutdatedSecurityPolicy(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        UA_ByteString basic128uri =
            UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
        UA_ByteString basic256uri =
            UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
        if(!UA_String_equal(&ep->securityPolicyUri, &basic128uri) &&
           !UA_String_equal(&ep->securityPolicyUri, &basic256uri))
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize - 1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize == 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

#endif

/* This access control callback checks the user access on the SecurityGroup object, when
 * GetSecurityKeys method is called. */
static UA_Boolean
getUserExecutableOnObject_sks(UA_Server *server, UA_AccessControl *ac,
                              const UA_NodeId *sessionId, void *sessionContext,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext) {
    if(objectContext && sessionContext) {
        UA_ByteString *username = (UA_ByteString *)objectContext;
        UA_ByteString *sessionUsername = (UA_ByteString *)sessionContext;
        if(!UA_ByteString_equal(username, sessionUsername))
            return false;
    }
    return true;
}

/**
 * We need to add a SecurityGroup for the management of security keys on SKS server.
 * The publishers/subcribers can requests the keys on SKS with their associated
 * security groups. The SKS will check if the user credentials used to establish
 * the session have the access to the requested security group managed by SKS.
 */
static void
addSecurityGroup(UA_Server *server, UA_NodeId *outNodeId) {
    UA_Duration keyLifeTimeMinutes = DEMO_KEYLIFETIME_MINUTES;
    UA_UInt32 maxFutureKeyCount = DEMO_MAXFUTUREKEYCOUNT;
    UA_UInt32 maxPastKeyCount = DEMO_MAXPASTKEYCOUNT;
    char *securityGroupName = DEMO_SECURITYGROUPNAME;
    UA_NodeId securityGroupParent = UA_NS0ID(PUBLISHSUBSCRIBE_SECURITYGROUPS);

    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = keyLifeTimeMinutes * MINUTE_SECONDS * MILLI_SECONDS;
    config.securityPolicyUri = UA_STRING(policUri);
    config.securityGroupName = UA_STRING(securityGroupName);
    config.maxFutureKeyCount = maxFutureKeyCount;
    config.maxPastKeyCount = maxPastKeyCount;

    UA_Server_addSecurityGroup(server, securityGroupParent, &config, outNodeId);
}

/*
 * we need to set user access for the security groups. The allowed users can be
 * set in the node context of the SecurityGroup Object Node, which are checked
 * are checked by access control plugin.
 */
static UA_StatusCode
setSecurityGroupRolePermission(UA_Server *server, UA_NodeId securityGroupNodeId,
                               void *nodeContext) {
    if(!server && !nodeContext)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ByteString allowedUsername = UA_STRING((char *)nodeContext);
    return UA_Server_setNodeContext(server, securityGroupNodeId, &allowedUsername);
}

static void
usage(char *progname) {
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "Usage:\n"
#ifndef UA_ENABLE_ENCRYPTION
                   "%s [<server-certificate.der>]\n"
#else
                   "%s <server-certificate.der> <private-key.der>\n"
#ifndef __linux__
                   "\t[--trustlist <tl1.ctl> <tl2.ctl> ... ]\n"
                   "\t[--issuerlist <il1.der> <il2.der> ... ]\n"
                   "\t[--revocationlist <rv1.crl> <rv2.crl> ...]\n"
#else
                   "\t[--trustlistFolder <folder>]\n"
                   "\t[--issuerlistFolder <folder>]\n"
                   "\t[--revocationlistFolder <folder>]\n"
#endif
                   "\t[--enableUnencrypted]\n"
                   "\t[--enableOutdatedSecurityPolicy]\n"
#endif
                   "\t[--enableTimestampCheck]\n"
                   "\t[--enableAnonymous]\n",
                   progname);
}

int
main(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));

    /* Load certificate */
    size_t pos = 1;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        certificate = loadFile(argv[1]);
        if(certificate.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

#ifdef UA_ENABLE_ENCRYPTION
    /* Load the private key */
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        privateKey = loadFile(argv[2]);
        if(privateKey.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

    char filetype = ' '; /* t==trustlist, l == issuerList, r==revocationlist */
    UA_Boolean enableUnencr = false;
    UA_Boolean enableSec = false;

#ifndef __linux__
    UA_ByteString trustList[100];
    size_t trustListSize = 0;
    UA_ByteString issuerList[100];
    size_t issuerListSize = 0;
    UA_ByteString revocationList[100];
    size_t revocationListSize = 0;
#else
    char *pkiFolder = NULL;
#endif /* __linux__ */

#endif /* UA_ENABLE_ENCRYPTION */

    UA_Boolean enableAnon = false;
    UA_Boolean enableTime = false;
    UA_UInt16 port = 4840;

    /* Loop over the remaining arguments */
    for(; pos < (size_t)argc; pos++) {

        if(strcmp(argv[pos], "--port") == 0) {
            pos++;
            int inNum = atoi(argv[pos]);
            if(inNum <= 0) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }

            port = (UA_UInt16)inNum;
            continue;
        }

        if(strcmp(argv[pos], "--enableAnonymous") == 0) {
            enableAnon = true;
            continue;
        }

        if(strcmp(argv[pos], "--enableTimestampCheck") == 0) {
            enableTime = true;
            continue;
        }

#ifdef UA_ENABLE_ENCRYPTION
        if(strcmp(argv[pos], "--enableUnencrypted") == 0) {
            enableUnencr = true;
            continue;
        }

        if(strcmp(argv[pos], "--enableOutdatedSecurityPolicy") == 0) {
            enableSec = true;
            continue;
        }

#ifndef __linux__
        if(strcmp(argv[pos], "--trustlist") == 0) {
            filetype = 't';
            continue;
        }

        if(strcmp(argv[pos], "--issuerlist") == 0) {
            filetype = 'l';
            continue;
        }

        if(strcmp(argv[pos], "--revocationlist") == 0) {
            filetype = 'r';
            continue;
        }

        if(filetype == 't') {
            if(trustListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many trust lists");
                return EXIT_FAILURE;
            }
            trustList[trustListSize] = loadFile(argv[pos]);
            if(trustList[trustListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load trust list %s", argv[pos]);
                return EXIT_FAILURE;
            }
            trustListSize++;
            continue;
        }

        if(filetype == 'l') {
            if(issuerListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many trust lists");
                return EXIT_FAILURE;
            }
            issuerList[issuerListSize] = loadFile(argv[pos]);
            if(issuerList[issuerListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load trust list %s", argv[pos]);
                return EXIT_FAILURE;
            }
            issuerListSize++;
            continue;
        }

        if(filetype == 'r') {
            if(revocationListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many revocation lists");
                return EXIT_FAILURE;
            }
            revocationList[revocationListSize] = loadFile(argv[pos]);
            if(revocationList[revocationListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load revocationlist %s", argv[pos]);
                return EXIT_FAILURE;
            }
            revocationListSize++;
            continue;
        }
#else  /* __linux__ */
        if(strcmp(argv[pos], "--pkiFolder") == 0) {
            filetype = 't';
            continue;
        }

        if(filetype == 't') {
            pkiFolder = argv[pos];
            continue;
        }
#endif /* __linux__ */

#endif /* UA_ENABLE_ENCRYPTION */

        usage(argv[0]);
        return EXIT_FAILURE;
    }

    UA_Server *server = NULL;

#ifdef UA_ENABLE_ENCRYPTION
#ifndef __linux__
    UA_StatusCode res = UA_ServerConfig_setDefaultWithSecurityPolicies(
        &config, port, &certificate, &privateKey, trustList, trustListSize, issuerList,
        issuerListSize, revocationList, revocationListSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#else /* On Linux we can monitor the pki folder and reload when changes are made */
    if(!pkiFolder) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                 "Path to the Pki folder must be specified.");
        goto cleanup;
    }
    UA_String pkiStoreFolder = UA_STRING(pkiFolder);
    UA_StatusCode res = UA_ServerConfig_setDefaultWithFilestore(
        &config, port, &certificate, &privateKey, pkiStoreFolder);
    UA_String_clear(&pkiStoreFolder);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif /* __linux__ */

    if(!enableUnencr)
        disableUnencrypted(&config);
    if(!enableSec)
        disableOutdatedSecurityPolicy(&config);

#else  /* UA_ENABLE_ENCRYPTION */
    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, port, &certificate);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif /* UA_ENABLE_ENCRYPTION */

    if(!enableAnon)
        disableAnonymous(&config);

    /* Limit the number of SecureChannels and Sessions */
    config.maxSecureChannels = 10;
    config.maxSessions = 20;

    /* Revolve the SecureChannel token every 300 seconds */
    config.maxSecurityTokenLifetime = 300000;

    /* Set operation limits */
    config.maxNodesPerRead = MAX_OPERATION_LIMIT;
    config.maxNodesPerWrite = MAX_OPERATION_LIMIT;
    config.maxNodesPerMethodCall = MAX_OPERATION_LIMIT;
    config.maxNodesPerBrowse = MAX_OPERATION_LIMIT;
    config.maxNodesPerRegisterNodes = MAX_OPERATION_LIMIT;
    config.maxNodesPerTranslateBrowsePathsToNodeIds = MAX_OPERATION_LIMIT;
    config.maxNodesPerNodeManagement = MAX_OPERATION_LIMIT;
    config.maxMonitoredItemsPerCall = MAX_OPERATION_LIMIT;

    /* Set Subscription limits */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    config.maxSubscriptions = 20;
#endif

    /* If RequestTimestamp is '0', log the warning and proceed */
    config.verifyRequestTimestamp = UA_RULEHANDLING_WARN;
    if(enableTime)
        config.verifyRequestTimestamp = UA_RULEHANDLING_DEFAULT;

    /* Override with a custom access control policy */
    UA_String_clear(&config.applicationDescription.applicationUri);
    config.applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.server.application");

    config.shutdownDelay = 5000.0; /* 5s */

    /* Add supported pubsub security policies by this sks instance */
    config.pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config.pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config.pubSubConfig.securityPolicies,
                                      config.logging);

    /* User Access Control */
    config.accessControl.getUserExecutableOnObject = getUserExecutableOnObject_sks;

    server = UA_Server_newWithConfig(&config);
    if(!server) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    UA_NodeId outNodeId;
    addSecurityGroup(server, &outNodeId);

    char *username = "user1";
    setSecurityGroupRolePermission(server, outNodeId, username);

    UA_Server_enableAllPubSubComponents(server);
    UA_Server_runUntilInterrupt(server);

cleanup:
    if(server)
        UA_Server_delete(server);
    else
        UA_ServerConfig_clear(&config);

    UA_ByteString_clear(&certificate);
#if defined(UA_ENABLE_ENCRYPTION)
    UA_ByteString_clear(&privateKey);
#ifndef __linux__
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    for(size_t i = 0; i < issuerListSize; i++)
        UA_ByteString_clear(&issuerList[i]);
    for(size_t i = 0; i < revocationListSize; i++)
        UA_ByteString_clear(&revocationList[i]);
#endif
#endif

    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
