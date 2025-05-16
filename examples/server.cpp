/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

//  ./server D:\OPC UA Server\OPCUA- open 62451\open62541-C\certs\server_cert.der D:\OPC UA Server\OPCUA- open 62451\open62541-C\certs\server_key.der [trust1.der trust2.der ...]
//  ./server D:\OPC UA Server\OPCUA- open 62451\open62541-C\certs\server_cert.der D:\OPC UA Server\OPCUA- open 62451\open62541-C\certs\server_key.der

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/securitypolicy.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Build Instructions (Linux)
 * - g++ server.cpp -lopen62541 -o server */

using namespace std;

UA_Boolean running = true;

// Define user credentials
static UA_UsernamePasswordLogin usernamePasswordLogin[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password1")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password2")}
};

// Custom access control
static UA_ByteString
getPassword(const UA_String *userName, void *userContext) {
    if(UA_String_equal(userName, &usernamePasswordLogin[0].username))
        return usernamePasswordLogin[0].password;
    if(UA_String_equal(userName, &usernamePasswordLogin[1].username))
        return usernamePasswordLogin[1].password;
    return UA_BYTESTRING_NULL;
}

static UA_StatusCode
myLoginCallback(const UA_String *username, const UA_ByteString *password,
                size_t usernamePasswordLoginSize,
                const UA_UsernamePasswordLogin *usernamePasswordLogin,
                void **sessionContext, void *loginContext) {
    for(size_t i = 0; i < usernamePasswordLoginSize; i++) {
        if(UA_String_equal(username, &usernamePasswordLogin[i].username) &&
           UA_ByteString_equal(password, &usernamePasswordLogin[i].password)) {
            return UA_STATUSCODE_GOOD;
        }
    }
    // Log failed login attempt
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Login failed for user: %.*s",
                   (int)username->length, username->data);
    return UA_STATUSCODE_BADUSERACCESSDENIED;
}

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "stopping server");
}

static UA_ByteString loadFile(const char *path) {
    UA_ByteString fileContents = UA_BYTESTRING_NULL;
    FILE *fp = fopen(path, "rb");
    if(!fp)
        return fileContents;
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    fseek(fp, 0, SEEK_SET);
    if(fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp) != fileContents.length) {
        UA_ByteString_clear(&fileContents);
    }
    fclose(fp);
    return fileContents;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    size_t trustListSize = 0;
    UA_ByteString *trustList = NULL;

    if(argc >= 3) {
        certificate = loadFile(argv[1]);
        privateKey = loadFile(argv[2]);
        trustListSize = (size_t)argc-3;
        if(trustListSize > 0) {
            trustList = (UA_ByteString*)UA_malloc(sizeof(UA_ByteString)*trustListSize);
            for(size_t i = 0; i < trustListSize; i++)
                trustList[i] = loadFile(argv[i+3]);
        }
    } else {
        // Fallback: run with no security
        certificate = UA_BYTESTRING_NULL;
        privateKey = UA_BYTESTRING_NULL;
        trustListSize = 0;
        trustList = NULL;
    }

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

// #ifdef UA_ENABLE_ENCRYPTION
    // UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey, trustList, trustListSize, NULL, 0, NULL, 0);
        UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey, NULL, NULL, NULL, 0, NULL, 0);
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:Anexee.server.application");
    // Accept all certificates for demo
    config->secureChannelPKI.clear(&config->secureChannelPKI);
    // UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    config->sessionPKI.clear(&config->sessionPKI);
    // UA_CertificateGroup_AcceptAll(&config->sessionPKI);
// #else
//     UA_ServerConfig_setMinimal(config, 4840, &certificate);
// #endif

    UA_AccessControl_defaultWithLoginCallback(config, true, NULL, 2, usernamePasswordLogin, myLoginCallback, NULL);
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;

    // // Set up multiple endpoints with different security policies
    // config->endpointsSize = 3;
    // config->endpoints = (UA_EndpointDescription*)UA_Array_new(3, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    
    // // Endpoint 1: None (for testing)
    // UA_EndpointDescription_init(&config->endpoints[0]);
    // config->endpoints[0].endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    // config->endpoints[0].securityMode = UA_MESSAGESECURITYMODE_NONE;
    // config->endpoints[0].securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    
    // // Endpoint 2: Sign
    // UA_EndpointDescription_init(&config->endpoints[1]);
    // config->endpoints[1].endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4841");
    // config->endpoints[1].securityMode = UA_MESSAGESECURITYMODE_SIGN;
    // config->endpoints[1].securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    
    // // Endpoint 3: Sign & Encrypt
    // UA_EndpointDescription_init(&config->endpoints[2]);
    // config->endpoints[2].endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4842");
    // config->endpoints[2].securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    // config->endpoints[2].securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    // Add server description
    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:Anexee.server.application");
    config->applicationDescription.productUri = UA_STRING_ALLOC("urn:Anexee.server");
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en-US", "Anexee");

    // add a variable node to the adresspace
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL, NULL);

    // Add a second variable that changes periodically
    UA_VariableAttributes attr2 = UA_VariableAttributes_default;
    UA_Double myDouble = 0.0;
    UA_Variant_setScalarCopy(&attr2.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr2.description = UA_LOCALIZEDTEXT_ALLOC("en-US","counter");
    attr2.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","counter");
    attr2.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myDoubleNodeId = UA_NODEID_STRING_ALLOC(1, "counter");
    UA_QualifiedName myDoubleName = UA_QUALIFIEDNAME_ALLOC(1, "counter");
    UA_Server_addVariableNode(server, myDoubleNodeId, parentNodeId,
                             parentReferenceNodeId, myDoubleName,
                             UA_NODEID_NULL, attr2, NULL, NULL);

    /* allocations on the heap need to be freed */
    UA_VariableAttributes_clear(&attr);
    UA_VariableAttributes_clear(&attr2);
    UA_NodeId_clear(&myIntegerNodeId);
    UA_NodeId_clear(&myDoubleNodeId);
    UA_QualifiedName_clear(&myIntegerName);
    UA_QualifiedName_clear(&myDoubleName);

    /* Create a rudimentary objectType hierarchy
     * BaseObjectType
     * |
     * +- (OT) AnimalType
     *    + (V) Age
     *    + (OT) DogType
     *      |
     *      + (V) Name
     */

    // Create AnimalType
    UA_ObjectTypeAttributes animalTypeAttr = UA_ObjectTypeAttributes_default;
    animalTypeAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "AnimalType");
    animalTypeAttr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "A base type for all animals");
    UA_NodeId animalTypeId = UA_NODEID_STRING_ALLOC(1, "AnimalType");
    UA_QualifiedName animalTypeName = UA_QUALIFIEDNAME_ALLOC(1, "AnimalType");
    UA_Server_addObjectTypeNode(server, animalTypeId,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                               animalTypeName, animalTypeAttr, NULL, NULL);

    // Add Age variable to AnimalType
    UA_VariableAttributes ageAttr = UA_VariableAttributes_default;
    UA_Int32 age = 0;
    UA_Variant_setScalarCopy(&ageAttr.value, &age, &UA_TYPES[UA_TYPES_INT32]);
    ageAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "Age");
    ageAttr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The age of the animal");
    ageAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId ageNodeId = UA_NODEID_STRING_ALLOC(1, "AnimalType.Age");
    UA_QualifiedName ageName = UA_QUALIFIEDNAME_ALLOC(1, "Age");
    UA_Server_addVariableNode(server, ageNodeId, animalTypeId,
                             UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                             ageName, UA_NODEID_NULL, ageAttr, NULL, NULL);

    // Create DogType
    UA_ObjectTypeAttributes dogTypeAttr = UA_ObjectTypeAttributes_default;
    dogTypeAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "DogType");
    dogTypeAttr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "A type for dogs");
    UA_NodeId dogTypeId = UA_NODEID_STRING_ALLOC(1, "DogType");
    UA_QualifiedName dogTypeName = UA_QUALIFIEDNAME_ALLOC(1, "DogType");
    UA_Server_addObjectTypeNode(server, dogTypeId, animalTypeId,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                               dogTypeName, dogTypeAttr, NULL, NULL);

    // Add Name variable to DogType
    UA_VariableAttributes nameAttr = UA_VariableAttributes_default;
    UA_String name = UA_STRING_ALLOC("Unknown");
    UA_Variant_setScalarCopy(&nameAttr.value, &name, &UA_TYPES[UA_TYPES_STRING]);
    nameAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "Name");
    nameAttr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The name of the dog");
    nameAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId nameNodeId = UA_NODEID_STRING_ALLOC(1, "DogType.Name");
    UA_QualifiedName nameName = UA_QUALIFIEDNAME_ALLOC(1, "Name");
    UA_Server_addVariableNode(server, nameNodeId, dogTypeId,
                             UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                             nameName, UA_NODEID_NULL, nameAttr, NULL, NULL);

    // Create an instance of DogType
    UA_ObjectAttributes dogInstanceAttr = UA_ObjectAttributes_default;
    dogInstanceAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "MyDog");
    dogInstanceAttr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "An instance of a dog");
    UA_NodeId dogInstanceId = UA_NODEID_STRING_ALLOC(1, "MyDog");
    UA_QualifiedName dogInstanceName = UA_QUALIFIEDNAME_ALLOC(1, "MyDog");
    UA_Server_addObjectNode(server, dogInstanceId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                           dogInstanceName, dogTypeId, dogInstanceAttr, NULL, NULL);

    // Clean up allocated resources
    UA_ObjectTypeAttributes_clear(&animalTypeAttr);
    UA_ObjectTypeAttributes_clear(&dogTypeAttr);
    UA_ObjectAttributes_clear(&dogInstanceAttr);
    UA_VariableAttributes_clear(&ageAttr);
    UA_VariableAttributes_clear(&nameAttr);
    UA_NodeId_clear(&animalTypeId);
    UA_NodeId_clear(&dogTypeId);
    UA_NodeId_clear(&dogInstanceId);
    UA_NodeId_clear(&ageNodeId);
    UA_NodeId_clear(&nameNodeId);
    UA_QualifiedName_clear(&animalTypeName);
    UA_QualifiedName_clear(&dogTypeName);
    UA_QualifiedName_clear(&dogInstanceName);
    UA_QualifiedName_clear(&ageName);
    UA_QualifiedName_clear(&nameName);
    UA_String_clear(&name);

    UA_StatusCode retval = UA_Server_run(server, &running);

    // Clean up security policies
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        config->securityPolicies[i].clear(&config->securityPolicies[i]);
    }
    if(trustList) {
        for(size_t i = 0; i < trustListSize; i++)
            UA_ByteString_clear(&trustList[i]);
        UA_free(trustList);
    }
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}