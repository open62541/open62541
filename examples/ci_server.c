/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER
# include <open62541/driver/gds_receiver.h>
#endif

#include <stdlib.h>

#include "common.h"

/* This is a test server to the ci script. It can be used for some of the examples that need a server to connect.
* It allows to connect with the username "peter" and "paula" and the password "peter123" and "paula123" or "user1" and "password". Anonymus login is also allowed.
* The server has a method "hello world" and a variable "the answer" that can be written to.
* The server certificate and private key are loaded from the command line arguments.
*/

static UA_UsernamePasswordLogin logins[3] = {
    {UA_STRING_STATIC("peter"), UA_STRING_STATIC("peter123")},
    {UA_STRING_STATIC("paula"), UA_STRING_STATIC("paula123")},
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")}
};

#ifdef UA_ENABLE_RBAC
#include <string.h>

static void
persistRolesToFile(const char *filePath, const UA_Role *roles, size_t rolesSize) {
    FILE *fp = fopen(filePath, "wb");
    if(!fp)
        return;

    uint32_t count = (uint32_t)rolesSize;
    fwrite(&count, sizeof(uint32_t), 1, fp);

    for(size_t i = 0; i < rolesSize; i++) {
        const UA_Role *r = &roles[i];

        UA_ByteString bId = UA_BYTESTRING_NULL;
        UA_encodeBinary(&r->roleId, &UA_TYPES[UA_TYPES_NODEID], &bId, NULL);
        uint32_t len = (uint32_t)bId.length;
        fwrite(&len, sizeof(uint32_t), 1, fp);
        if(len > 0) fwrite(bId.data, 1, len, fp);
        UA_ByteString_clear(&bId);

        UA_ByteString bName = UA_BYTESTRING_NULL;
        UA_encodeBinary(&r->roleName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &bName, NULL);
        len = (uint32_t)bName.length;
        fwrite(&len, sizeof(uint32_t), 1, fp);
        if(len > 0) fwrite(bName.data, 1, len, fp);
        UA_ByteString_clear(&bName);

        uint32_t rulesSize = (uint32_t)r->identityMappingRulesSize;
        fwrite(&rulesSize, sizeof(uint32_t), 1, fp);
        for(size_t j = 0; j < r->identityMappingRulesSize; j++) {
            UA_ByteString bRule = UA_BYTESTRING_NULL;
            UA_encodeBinary(&r->identityMappingRules[j], &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE], &bRule, NULL);
            len = (uint32_t)bRule.length;
            fwrite(&len, sizeof(uint32_t), 1, fp);
            if(len > 0) fwrite(bRule.data, 1, len, fp);
            UA_ByteString_clear(&bRule);
        }

        uint8_t appEx = (uint8_t)r->applicationsExclude;
        fwrite(&appEx, sizeof(uint8_t), 1, fp);
        uint32_t appsSize = (uint32_t)r->applicationsSize;
        fwrite(&appsSize, sizeof(uint32_t), 1, fp);
        for(size_t j = 0; j < r->applicationsSize; j++) {
            UA_ByteString bApp = UA_BYTESTRING_NULL;
            UA_encodeBinary(&r->applications[j], &UA_TYPES[UA_TYPES_STRING], &bApp, NULL);
            len = (uint32_t)bApp.length;
            fwrite(&len, sizeof(uint32_t), 1, fp);
            if(len > 0) fwrite(bApp.data, 1, len, fp);
            UA_ByteString_clear(&bApp);
        }

        uint8_t endEx = (uint8_t)r->endpointsExclude;
        fwrite(&endEx, sizeof(uint8_t), 1, fp);
        uint32_t endsSize = (uint32_t)r->endpointsSize;
        fwrite(&endsSize, sizeof(uint32_t), 1, fp);
        for(size_t j = 0; j < r->endpointsSize; j++) {
            UA_ByteString bEnd = UA_BYTESTRING_NULL;
            UA_encodeBinary(&r->endpoints[j], &UA_TYPES[UA_TYPES_ENDPOINTTYPE], &bEnd, NULL);
            len = (uint32_t)bEnd.length;
            fwrite(&len, sizeof(uint32_t), 1, fp);
            if(len > 0) fwrite(bEnd.data, 1, len, fp);
            UA_ByteString_clear(&bEnd);
        }
    }

    fclose(fp);
}

static void
loadPersistentRolesFromFile(UA_Server *server, const char *filePath) {
    FILE *fp = fopen(filePath, "rb");
    if(!fp)
        return;

    uint32_t count = 0;
    if(fread(&count, sizeof(uint32_t), 1, fp) != 1) {
        fclose(fp);
        return;
    }

    for(uint32_t i = 0; i < count; i++) {
        UA_Role r;
        UA_Role_init(&r);

        uint32_t len = 0;

        if(fread(&len, sizeof(uint32_t), 1, fp) != 1) break;
        if(len > 0) {
            UA_ByteString bId;
            UA_ByteString_init(&bId);
            UA_ByteString_allocBuffer(&bId, len);
            if(fread(bId.data, 1, len, fp) == len) {
                UA_decodeBinary(&bId, &r.roleId, &UA_TYPES[UA_TYPES_NODEID], NULL);
            }
            UA_ByteString_clear(&bId);
        }

        if(fread(&len, sizeof(uint32_t), 1, fp) != 1) break;
        if(len > 0) {
            UA_ByteString bName;
            UA_ByteString_init(&bName);
            UA_ByteString_allocBuffer(&bName, len);
            if(fread(bName.data, 1, len, fp) == len) {
                UA_decodeBinary(&bName, &r.roleName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], NULL);
            }
            UA_ByteString_clear(&bName);
        }

        uint32_t rulesSize = 0;
        if(fread(&rulesSize, sizeof(uint32_t), 1, fp) != 1) break;
        if(rulesSize > 0) {
            r.identityMappingRules = (UA_IdentityMappingRuleType*)UA_calloc(rulesSize, sizeof(UA_IdentityMappingRuleType));
            r.identityMappingRulesSize = rulesSize;
            for(uint32_t j = 0; j < rulesSize; j++) {
                if(fread(&len, sizeof(uint32_t), 1, fp) != 1) break;
                if(len > 0) {
                    UA_ByteString bRule;
                    UA_ByteString_init(&bRule);
                    UA_ByteString_allocBuffer(&bRule, len);
                    if(fread(bRule.data, 1, len, fp) == len) {
                        UA_decodeBinary(&bRule, &r.identityMappingRules[j], &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE], NULL);
                    }
                    UA_ByteString_clear(&bRule);
                }
            }
        }

        uint8_t appEx = 0;
        if(fread(&appEx, sizeof(uint8_t), 1, fp) != 1) break;
        r.applicationsExclude = (UA_Boolean)appEx;

        uint32_t appsSize = 0;
        if(fread(&appsSize, sizeof(uint32_t), 1, fp) != 1) break;
        if(appsSize > 0) {
            r.applications = (UA_String*)UA_calloc(appsSize, sizeof(UA_String));
            r.applicationsSize = appsSize;
            for(uint32_t j = 0; j < appsSize; j++) {
                if(fread(&len, sizeof(uint32_t), 1, fp) != 1) break;
                if(len > 0) {
                    UA_ByteString bApp;
                    UA_ByteString_init(&bApp);
                    UA_ByteString_allocBuffer(&bApp, len);
                    if(fread(bApp.data, 1, len, fp) == len) {
                        UA_decodeBinary(&bApp, &r.applications[j], &UA_TYPES[UA_TYPES_STRING], NULL);
                    }
                    UA_ByteString_clear(&bApp);
                }
            }
        }

        uint8_t endEx = 0;
        if(fread(&endEx, sizeof(uint8_t), 1, fp) != 1) break;
        r.endpointsExclude = (UA_Boolean)endEx;

        uint32_t endsSize = 0;
        if(fread(&endsSize, sizeof(uint32_t), 1, fp) != 1) break;
        if(endsSize > 0) {
            r.endpoints = (UA_EndpointType*)UA_calloc(endsSize, sizeof(UA_EndpointType));
            r.endpointsSize = endsSize;
            for(uint32_t j = 0; j < endsSize; j++) {
                if(fread(&len, sizeof(uint32_t), 1, fp) != 1) break;
                if(len > 0) {
                    UA_ByteString bEnd;
                    UA_ByteString_init(&bEnd);
                    UA_ByteString_allocBuffer(&bEnd, len);
                    if(fread(bEnd.data, 1, len, fp) == len) {
                        UA_decodeBinary(&bEnd, &r.endpoints[j], &UA_TYPES[UA_TYPES_ENDPOINTTYPE], NULL);
                    }
                    UA_ByteString_clear(&bEnd);
                }
            }
        }

        UA_Role existing;
        UA_Role_init(&existing);
        if(UA_Server_getRoleById(server, r.roleId, &existing) == UA_STATUSCODE_GOOD) {
            UA_Server_updateRole(server, &r);
            UA_Role_clear(&existing);
        } else {
            UA_Server_addRole(server, &r, NULL);
        }
        UA_Role_clear(&r);
    }

    fclose(fp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RBAC: Loaded persistent roles from disk successfully");
}

static void
checkAndPersistRolesCallback(UA_Server *server, void *data) {
    (void)data;
    size_t roleNamesSize = 0;
    UA_QualifiedName *roleNames = NULL;
    
    UA_StatusCode retval = UA_Server_getRoles(server, &roleNamesSize, &roleNames);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    UA_Role *roles = (UA_Role*)UA_calloc(roleNamesSize, sizeof(UA_Role));
    if(!roles) {
        UA_Array_delete(roleNames, roleNamesSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        return;
    }

    size_t rolesSize = 0;
    for(size_t i = 0; i < roleNamesSize; i++) {
        if(UA_Server_getRole(server, roleNames[i], &roles[rolesSize]) == UA_STATUSCODE_GOOD) {
            rolesSize++;
        }
    }

    const char *pkiDir = getenv("UA_PKI_DIR");
    if(pkiDir && strlen(pkiDir) > 0) {
        char filePath[512];
        snprintf(filePath, sizeof(filePath), "%s/roles_persistent.bin", pkiDir);
        
        persistRolesToFile(filePath, roles, rolesSize);
    }

    for(size_t i = 0; i < rolesSize; i++) {
        UA_Role_clear(&roles[i]);
    }
    UA_free(roles);
    UA_Array_delete(roleNames, roleNamesSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
}

static void
loadPersistentRolesOnBoot(UA_Server *server) {
    const char *pkiDir = getenv("UA_PKI_DIR");
    if(!pkiDir || strlen(pkiDir) == 0)
        return;

    char filePath[512];
    snprintf(filePath, sizeof(filePath), "%s/roles_persistent.bin", pkiDir);
    
    loadPersistentRolesFromFile(server, filePath);
}
#endif

static UA_StatusCode
helloWorldMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

static void
addHelloWorldMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NS0ID(BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void
writeVariable(UA_Server *server) {
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");

    /* Write a different integer value */
    UA_Int32 myInteger = 43;
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, myIntegerNodeId, myVar);

    /* Set the status code of the value to an error code. The function
     * UA_Server_write provides access to the raw service. The above
     * UA_Server_writeValue is syntactic sugar for writing a specific node
     * attribute with the write service. */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myIntegerNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    /* Reset the variable to a good statuscode with a value */
    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);
}

static void
customLogCallback(void *logContext, UA_LogLevel level, UA_LogCategory category,
                  const char *msg, va_list args) {
    if(category == UA_LOGCATEGORY_EVENTLOOP) {
        return;
    }
    UA_Log_Stdout->log(logContext, level, category, msg, args);
}

static void
setupLogger(UA_ServerConfig *config) {
    if(!config || !config->logging) {
        return;
    }
    const char *envLogLevel = getenv("UA_LOG_LEVEL");
    UA_LogLevel minLogLevel = UA_LOGLEVEL_INFO;
    if(envLogLevel) {
        if(strcmp(envLogLevel, "TRACE") == 0) {
            minLogLevel = UA_LOGLEVEL_TRACE;
        } else if(strcmp(envLogLevel, "DEBUG") == 0) {
            minLogLevel = UA_LOGLEVEL_DEBUG;
        } else if(strcmp(envLogLevel, "INFO") == 0) {
            minLogLevel = UA_LOGLEVEL_INFO;
        } else if(strcmp(envLogLevel, "WARNING") == 0) {
            minLogLevel = UA_LOGLEVEL_WARNING;
        } else if(strcmp(envLogLevel, "ERROR") == 0) {
            minLogLevel = UA_LOGLEVEL_ERROR;
        } else if(strcmp(envLogLevel, "FATAL") == 0) {
            minLogLevel = UA_LOGLEVEL_FATAL;
        }
    }
    UA_Logger logger;
    logger.log = customLogCallback;
    logger.context = (void*)(uintptr_t)minLogLevel;
    logger.clear = config->logging->clear;
    *config->logging = logger;
}

#ifdef UA_ENABLE_ENCRYPTION
static void
removeDeprecatedSecurityPolicies(UA_ServerConfig *config) {
    if(!config) {
        return;
    }
    /* Remove deprecated and insecure security policies (Basic128Rsa15, Basic256) */
    size_t newPoliciesSize = 0;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_SecurityPolicy *policy = &config->securityPolicies[i];
        const UA_String basic128Uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
        const UA_String basic256Uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
        if(UA_String_equal(&policy->policyUri, &basic128Uri) ||
           UA_String_equal(&policy->policyUri, &basic256Uri)) {
            if(policy->clear) {
                policy->clear(policy);
            }
        } else {
            if(newPoliciesSize != i) {
                config->securityPolicies[newPoliciesSize] = *policy;
            }
            newPoliciesSize++;
        }
    }
    config->securityPoliciesSize = newPoliciesSize;

    /* Filter endpoints matching the removed security policies */
    size_t newEndpointsSize = 0;
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *endpoint = &config->endpoints[i];
        const UA_String basic128Uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
        const UA_String basic256Uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
        if(UA_String_equal(&endpoint->securityPolicyUri, &basic128Uri) ||
           UA_String_equal(&endpoint->securityPolicyUri, &basic256Uri)) {
            UA_EndpointDescription_clear(endpoint);
        } else {
            if(newEndpointsSize != i) {
                config->endpoints[newEndpointsSize] = *endpoint;
            }
            newEndpointsSize++;
        }
    }
    config->endpointsSize = newEndpointsSize;
}
#endif

int main(int argc, char* argv[]) {
    UA_StatusCode retval = 0;

    UA_Server *server = UA_Server_new();
#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER
    UA_GDSReceiver *gds_receiver = UA_GDSReceiver_new();
    if (gds_receiver) {
        UA_Server_addDriver(server, &gds_receiver->drv);
    }
#endif
    UA_ServerConfig *config = UA_Server_getConfig(server);

    setupLogger(config);

#ifdef UA_ENABLE_ENCRYPTION
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_UInt16 port = 0;
    if(argc >= 4) {
        /* Load port, certificate and private key */
        port = (UA_UInt16) atoi(argv[1]);
        certificate = loadFile(argv[2]);
        privateKey = loadFile(argv[3]);
        // print the certificat and private key
        printf("certificate: %.*s\n", (int)certificate.length, certificate.data);
    } else {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Missing arguments. Arguments are "
                     "<port> <server-certificate.der> <private-key.der> "
                     "[<trustlist1.crl>, ...]");

        return EXIT_SUCCESS;
    }

    /* Load the trustlist */
    size_t trustListSize = 0;
    if(argc > 4)
        trustListSize = (size_t)argc-4;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize+2);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i+4]);

    if(trustListSize == 0) {
        /* Trust our own certificate by default to initialize the secureChannelPKI and sessionPKI */
        UA_ByteString_copy(&certificate, &trustList[0]);
        trustListSize = 1;
    }

    /* Loading of an issuer list, not used in this application */
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;

    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    const char *pkiDir = getenv("UA_PKI_DIR");
    if(pkiDir && strlen(pkiDir) > 0) {
        UA_String storePath = UA_STRING((char*)(uintptr_t)pkiDir);
        retval = UA_ServerConfig_setDefaultWithFilestore(config, port,
                                                         &certificate, &privateKey,
                                                         storePath);
    } else
#endif
    {
        retval = UA_ServerConfig_setDefaultWithSecurityPolicies(config, port,
                                                          &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    }

    if(retval == UA_STATUSCODE_GOOD) {
        removeDeprecatedSecurityPolicies(config);
    }

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;   

#endif
    retval = UA_AccessControl_default(config, true,
             &config->securityPolicies[config->securityPoliciesSize-1].policyUri, 3, logins);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
        
    addHelloWorldMethod(server);
    addVariable(server);
    writeVariable(server);

#ifdef UA_ENABLE_RBAC
    /* 1. First, load any previously persisted roles and mapping rules from disk */
    loadPersistentRolesOnBoot(server);

    /* 2. Map user "peter" to the "SecurityAdmin" role for administrative GDS/RoleSet access if not already mapped */
    UA_NodeId secAdminId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    UA_Role secAdmin;
    if(UA_Server_getRoleById(server, secAdminId, &secAdmin) == UA_STATUSCODE_GOOD) {
        UA_Boolean alreadyMapped = false;
        UA_String targetUser = UA_STRING("peter");
        for(size_t i = 0; i < secAdmin.identityMappingRulesSize; i++) {
            if(secAdmin.identityMappingRules[i].criteriaType == UA_IDENTITYCRITERIATYPE_USERNAME &&
               UA_String_equal(&secAdmin.identityMappingRules[i].criteria, &targetUser)) {
                alreadyMapped = true;
                break;
            }
        }
        
        if(!alreadyMapped) {
            UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType *)
                UA_realloc(secAdmin.identityMappingRules,
                           (secAdmin.identityMappingRulesSize + 1) *
                           sizeof(UA_IdentityMappingRuleType));
            if(rules) {
                secAdmin.identityMappingRules = rules;
                UA_IdentityMappingRuleType_init(&rules[secAdmin.identityMappingRulesSize]);
                rules[secAdmin.identityMappingRulesSize].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
                rules[secAdmin.identityMappingRulesSize].criteria = UA_STRING_ALLOC("peter");
                secAdmin.identityMappingRulesSize++;
                UA_Server_updateRole(server, &secAdmin);
            }
        }
        UA_Role_clear(&secAdmin);
    }

    /* 3. Register a repeated callback to check for dynamic runtime RoleSet/identity mapping changes and persist them to disk */
    UA_UInt64 callbackId = 0;
    UA_Server_addRepeatedCallback(server, checkAndPersistRolesCallback, NULL, 5000, &callbackId);
#endif

    retval = UA_Server_runUntilInterrupt(server);

    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
 cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
