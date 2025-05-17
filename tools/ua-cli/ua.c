/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/plugin/log.h>
#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static UA_Client *client = NULL;
static UA_ClientConfig cc;
static char *url = NULL;
static char *username = NULL;
static char *password = NULL;
static UA_ByteString certificate;
static UA_ByteString privateKey;
static UA_ByteString securityPolicyUri;
int return_value = 0;

#define MAX_TOKENS 256
static char * tokens[MAX_TOKENS];
size_t tokenPos = 0;
size_t tokensSize = 0;

bool shellMode = false; /* How to abort */

/***********/
/* Logging */
/***********/

/* Custom logger that prints to stderr. So the "good output" can be easily separated. */
UA_LogLevel logLevel = UA_LOGLEVEL_ERROR;

/* ANSI escape sequences for color output taken from here:
 * https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static const char *
logLevelNames[6] = {"trace", "debug", ANSI_COLOR_GREEN "info",
                    ANSI_COLOR_YELLOW "warn", ANSI_COLOR_RED "error",
                    ANSI_COLOR_MAGENTA "fatal"};

static const char *
logCategoryNames[UA_LOGCATEGORIES] =
    {"network", "channel", "session", "server", "client",
     "userland", "security", "eventloop", "pubsub", "discovery"};

static void
cliLog(void *context, UA_LogLevel level, UA_LogCategory category,
       const char *msg, va_list args) {

    /* Set to fatal if the level is outside the range */
    int l = ((int)level / 100) - 1;
    if(l < 0 || l > 5)
        l = 5;

    if((int)logLevel - 1 > l)
        return;

    /* Log */
#define LOGBUFSIZE 512
    UA_Byte logbuf[LOGBUFSIZE];
    UA_String out = {LOGBUFSIZE, logbuf};
    UA_String_vprintf(&out, msg, args);
    fprintf(stderr, "%s/%s" ANSI_COLOR_RESET "\t",
           logLevelNames[l], logCategoryNames[category]);
    fprintf(stderr, "%s\n", logbuf);
    fflush(stderr);
}

UA_Logger stderrLog = {cliLog, NULL, NULL};

/******************/
/* Helper Methods */
/******************/

static void
abortWithStatus(UA_StatusCode res) {
    fprintf(stderr, "Error with StatusCode %s\n", UA_StatusCode_name(res));
    if(shellMode)
        return;

    if(client) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
    exit(res);
}

static void
abortWithMessage(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    if(shellMode)
        return;

    if(client) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
    exit(EXIT_FAILURE);
}

static void
usage(void) {
    if(shellMode) {
        fprintf(stderr, "Invalid input\n");
        return;
    }

    fprintf(stderr, "Usage: ua [--help | <options>] opc.tcp://domain[:port] [service]\n"
            " No service defined -> Shell taking repeated service calls\n"
            " service -> getendpoints: Print the endpoint descriptions of the server\n"
            " service -> read   <AttributeOperand>: Read an attribute\n"
            " service -> browse <AttributeOperand>: Browse the references of a node\n"
            " service -> write  <AttributeOperand> <value>: Write an attribute\n"
            " service -> explore <RelativePath> [--depth <int>]: Print the structure of the information model below the indicated node\n"
            //" <service> -> call <method-id> <object-id> <arguments>: Call the method \n"
            " Options:\n"
            " --username: Username for the session creation\n"
            " --password: Password for the session creation\n"
            " --certificate <certfile>: Certificate in DER format\n"
            " --privatekey <keyfile>: Private key in DER format\n"
            " --securitypolicy <policy-uri>: SecurityPolicy to be used\n"
            " --loglevel <level>: Logging detail [0 -> TRACE, 6 -> FATAL]\n"
            " --help: Print this message\n");
    exit(EXIT_FAILURE);
}

static UA_ByteString
loadFile(const char *const path) {
    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        fprintf(stderr, "Cannot open file %s\n", path);
        exit(EXIT_FAILURE);
    }

    /* Get the file length, allocate the data and read */
    UA_ByteString fileContents = UA_STRING_NULL;
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    fseek(fp, 0, SEEK_SET);
    size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
    if(read == 0)
        UA_ByteString_clear(&fileContents);
    fclose(fp);

    return fileContents;
}

static void
printType(void *p, const UA_DataType *type) {
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(UA_EncodeJsonOptions));
    opts.prettyPrint = true;
    opts.useReversible = true;
    opts.stringNodeIds = true;
    UA_StatusCode res = UA_encodeJson(p, type, &out, &opts);
    (void)res;
    printf("%.*s\n", (int)out.length, out.data);
    UA_ByteString_clear(&out);
}

static UA_StatusCode
parseVariant(UA_Variant *v, UA_String valstr);

static UA_StatusCode
parseVariantArray(UA_Variant *v, UA_String valstr, const UA_DataType *datatype) {
    v->type = datatype;
    bool hascomma = true;
    while(valstr.length > 0) {
        /* Skip space */
        if(isspace(*valstr.data)) {
            valstr.data++;
            valstr.length--;
            continue;
        }

        /* Comma (only a single comma allowed between elements) */
        if(*valstr.data == ',') {
            if(hascomma)
                return UA_STATUSCODE_BADDECODINGERROR;
            hascomma = true;
            valstr.data++;
            valstr.length--;
            continue;
        }

        /* Closing bracket, only whitespace allowed after */
        if(*valstr.data == ']') {
            for(size_t i = 0; i < valstr.length; i++) {
                if(!isspace(valstr.data[i]))
                    return UA_STATUSCODE_BADDECODINGERROR;
            }
            return UA_STATUSCODE_GOOD;
        }

        /* Allocate memory */
        void *data =
            UA_realloc(v->data, (v->arrayLength + 1) * datatype->memSize);
        if(!data)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        uintptr_t elem = (uintptr_t)data + (v->arrayLength * datatype->memSize);

        v->data = data;
        v->arrayLength++;

        /* Decode element */
        size_t jsonOffset = 0;
        UA_DecodeJsonOptions options;
        memset(&options, 0, sizeof(UA_DecodeJsonOptions));
        options.decodedLength = &jsonOffset;
        UA_StatusCode res =
            UA_decodeJson(&valstr, (void*)elem, &UA_TYPES[UA_TYPES_VARIANT], &options);
        if(res != UA_STATUSCODE_GOOD)
            return res;

        /* Move forward in the token */
        valstr.data += jsonOffset;
        valstr.length -= jsonOffset;
    }

    /* Array does not close with ] */
    return UA_STATUSCODE_BADDECODINGERROR;
}

static UA_StatusCode
parseVariant(UA_Variant *v, UA_String valstr) {
    /* Empty token */
    if(valstr.length == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Detect Boolean */
    UA_Boolean b;
    UA_String f = UA_STRING("false");
    UA_String t = UA_STRING("true");
    if(UA_String_equal(&valstr, &f)) {
        b = false;
        return UA_Variant_setScalarCopy(v, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
    } else if(UA_String_equal(&valstr, &t)) {
        b = true;
        return UA_Variant_setScalarCopy(v, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }

    /* Detect String */
    UA_String s = UA_STRING_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(valstr.data[0] == '\"' || valstr.data[0] == '\'') {
        UA_StatusCode res = UA_decodeJson(&valstr, &s, &UA_TYPES[UA_TYPES_STRING], NULL);
        res |= UA_Variant_setScalarCopy(v, &s, &UA_TYPES[UA_TYPES_STRING]);
        return res;
    }

    /* Detect integer and float */
    UA_Int32 i;
    UA_Float ff;
    if(valstr.data[0] == '.' || isdigit(valstr.data[0])) {
        res = UA_decodeJson(&valstr, &i, &UA_TYPES[UA_TYPES_INT32], NULL);
        if(res == UA_STATUSCODE_GOOD)
            return UA_Variant_setScalarCopy(v, &i, &UA_TYPES[UA_TYPES_INT32]);
        res = UA_decodeJson(&valstr, &ff, &UA_TYPES[UA_TYPES_FLOAT], NULL);
        res |= UA_Variant_setScalarCopy(v, &ff, &UA_TYPES[UA_TYPES_FLOAT]);
        return res;
    }

    /* Data type name in parentheses (default is Variant) */
    const UA_DataType *datatype = &UA_TYPES[UA_TYPES_VARIANT];
    if(valstr.data[0] == '(') {
        char typeString[512];
        UA_STACKARRAY(char, type, valstr.length);
        int elem = sscanf((char*)valstr.data, "(%511[^)])", type);
        if(elem <= 0) {
            abortWithMessage("Wrong datatype definition\n");
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        /* Find type under the name */
        size_t i = 0;
        for(; i < UA_TYPES_COUNT; i++) {
            if(strcmp(UA_TYPES[i].typeName, typeString) == 0) {
                datatype = &UA_TYPES[i];
                break;
            }
        }
        if(i == UA_TYPES_COUNT) {
            abortWithMessage("Data type %s unknown\n", type);
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        /* Advance beyond the datatype definition and more space */
        size_t advance = strlen(typeString) + 2;
        valstr.data += advance;
        valstr.length -= advance;
        while(valstr.length > 0 && isspace(valstr.data[0])) {
            valstr.data++;
            valstr.length--;
        }

        /* A value must remain */
        if(valstr.length == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Parse an array */
    if(valstr.data[0] == '[')
        return parseVariantArray(v, valstr, datatype);

    /* Parse as JSON */
    void *val = UA_new(datatype);
    if(!val)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    res = UA_decodeJson(&valstr, val, datatype, NULL);
    UA_Variant_setScalar(v, val, datatype);
    return UA_STATUSCODE_GOOD;
}

static void
connectClient(void) {
    UA_SecureChannelState channelState = UA_SECURECHANNELSTATE_CLOSED;
    UA_Client_getState(client, &channelState, NULL, NULL);
    if(channelState != UA_SECURECHANNELSTATE_CLOSED)
        return;

    UA_StatusCode res;
    if(username) {
        if(!password) {
            fprintf(stderr, "Username without password\n");
            exit(EXIT_FAILURE);
        }
        res = UA_Client_connectUsername(client, url, username, password);
    } else {
        res = UA_Client_connect(client, url);
    }
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);
}

/******************/
/* OPC UA Service */
/******************/

static void
getEndpoints(void) {
    /* Validate the arguments */
    if(tokenPos != tokensSize) {
        abortWithMessage("Arguments after \"getendpoints\" could not be parsed\n");
        return;
    }

    /* Get Endpoints */
    size_t endpointDescriptionsSize = 0;
    UA_EndpointDescription* endpointDescriptions = NULL;
    UA_StatusCode res = UA_Client_getEndpoints(client, url,
                                               &endpointDescriptionsSize,
                                               &endpointDescriptions);
    if(res != UA_STATUSCODE_GOOD) {
        abortWithStatus(res);
        return;
    }

    /* Print the results */
    UA_Variant var;
    UA_Variant_setArray(&var, endpointDescriptions, endpointDescriptionsSize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    printType(&var, &UA_TYPES[UA_TYPES_VARIANT]);

    /* Delete the allocated array */
    UA_Variant_clear(&var);
}

static void
readService(void) {
    /* Validate the arguments */
    if(tokenPos != tokensSize - 1) {
        abortWithMessage("The read service takes an AttributeOperand "
                         "expression as the last argument\n");
        return;
    }

    /* Connect */
    connectClient();

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(tokens[tokenPos]));
    if(res != UA_STATUSCODE_GOOD) {
        abortWithStatus(res);
        return;
    }

    /* Resolve the RelativePath */
    if(ao.browsePath.elementsSize > 0) {
        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.startingNode = ao.nodeId;
        bp.relativePath = ao.browsePath;

        UA_BrowsePathResult bpr =
            UA_Client_translateBrowsePathToNodeIds(client, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD)
            abortWithStatus(bpr.statusCode);

        /* Validate the response */
        if(bpr.targetsSize != 1) {
            abortWithMessage("The RelativePath did resolve to %u different NodeIds\n",
                             (unsigned)bpr.targetsSize);
            return;
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            abortWithMessage("The RelativePath was not fully resolved\n");
            return;
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            abortWithMessage("The RelativePath resolves to an ExpandedNodeId "
                             "on a different server\n");
            return;
        }

        UA_NodeId_clear(&ao.nodeId);
        ao.nodeId = bpr.targets[0].targetId.nodeId;
        UA_ExpandedNodeId_init(&bpr.targets[0].targetId);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Read the attribute */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = ao.nodeId;
    rvi.attributeId = ao.attributeId;
    rvi.indexRange = ao.indexRange;

    UA_DataValue resp = UA_Client_read(client, &rvi);
    printType(&resp, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_clear(&resp);
    UA_AttributeOperand_clear(&ao);
}

static void
writeService(void) {
    /* Validate the arguments */
    if(tokenPos + 1 >= tokensSize) {
        abortWithMessage("The Write Service takes an AttributeOperand "
                         "expression and the value as arguments\n");
        return;
    }

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(tokens[tokenPos++]));
    if(res != UA_STATUSCODE_GOOD) {
        abortWithStatus(res);
        return;
    }

    /* Aggregate all the remaining arguments into a single token */
    UA_String valstr = UA_STRING_NULL;
    for(; tokenPos < tokensSize; tokenPos++) {
        UA_String_append(&valstr, UA_STRING(tokens[tokenPos]));
        if(tokenPos != tokensSize - 1)
            UA_String_append(&valstr, UA_STRING(" "));
    }

    /* Parse the value */
    UA_Variant v;
    UA_Variant_init(&v);
    res = parseVariant(&v, valstr);
    UA_String_clear(&valstr);
    if(res != UA_STATUSCODE_GOOD) {
        abortWithMessage("Could not parse the value\n");
        return;
    }

    /* Connect */
    connectClient();

    /* Resolve the RelativePath */
    if(ao.browsePath.elementsSize > 0) {
        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.startingNode = ao.nodeId;
        bp.relativePath = ao.browsePath;

        UA_BrowsePathResult bpr =
            UA_Client_translateBrowsePathToNodeIds(client, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD)
            abortWithStatus(bpr.statusCode);

        /* Validate the response */
        if(bpr.targetsSize != 1) {
            abortWithMessage("The RelativePath did resolve to %u different NodeIds\n",
                             (unsigned)bpr.targetsSize);
            return;
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            abortWithMessage("The RelativePath was not fully resolved\n");
            return;
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            abortWithMessage("The RelativePath resolves to an ExpandedNodeId "
                             "on a different server\n");
            return;
        }

        UA_NodeId_clear(&ao.nodeId);
        ao.nodeId = bpr.targets[0].targetId.nodeId;
        UA_ExpandedNodeId_init(&bpr.targets[0].targetId);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Write the attribute */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.value.value = v;
    wv.value.hasValue = true;
    wv.nodeId = ao.nodeId;
    wv.attributeId = ao.attributeId;
    wv.indexRange = ao.indexRange;
    res = UA_Client_write(client, &wv);

    /* Print the StatusCode and return */
    fprintf(stdout, "%s\n", UA_StatusCode_name(res));
    if(res != UA_STATUSCODE_GOOD)
        return_value = EXIT_FAILURE;

    UA_AttributeOperand_clear(&ao);
    UA_Variant_clear(&v);
}

static void
browseService(void) {
    /* Validate the arguments */
    if(tokenPos != tokensSize - 1) {
        abortWithMessage("The browse service takes an AttributeOperand "
                         "expression as the last argument\n");
        return;
    }

    /* Connect */
    connectClient();

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(tokens[tokenPos]));
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);

    /* Resolve the RelativePath */
    if(ao.browsePath.elementsSize > 0) {
        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.startingNode = ao.nodeId;
        bp.relativePath = ao.browsePath;

        UA_BrowsePathResult bpr =
            UA_Client_translateBrowsePathToNodeIds(client, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD)
            abortWithStatus(bpr.statusCode);

        /* Validate the response */
        if(bpr.targetsSize != 1) {
            abortWithMessage("The RelativePath did resolve to %u different NodeIds\n",
                             (unsigned)bpr.targetsSize);
            return;
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            abortWithMessage("The RelativePath was not fully resolved\n");
            return;
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            abortWithMessage("The RelativePath resolves to an ExpandedNodeId "
                             "on a different server\n");
            return;
        }

        UA_NodeId_clear(&ao.nodeId);
        ao.nodeId = bpr.targets[0].targetId.nodeId;
        UA_ExpandedNodeId_init(&bpr.targets[0].targetId);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Read the attribute */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    bd.includeSubtypes = true;
    bd.nodeId = ao.nodeId;
    bd.referenceTypeId = UA_NS0ID(REFERENCES);
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Client_browse(client, NULL, 0, &bd);

    printType(&br, &UA_TYPES[UA_TYPES_BROWSERESULT]);
    UA_BrowseResult_clear(&br);
    UA_AttributeOperand_clear(&ao);
}

static char *nodeClassNames[] = {
    "Unspecified  ",
    "Object       ",
    "Variable     ",
    "Method       ",
    "ObjectType   ",
    "VariableType ",
    "ReferenceType",
    "DataType     ",
    "View         "
};

static void
exploreRecursive(char *pathString, size_t pos, const UA_NodeId current,
                 UA_NodeClass nc, size_t depth) {
    size_t targetlevel = 0;
    while(nc) {
        ++targetlevel;
        nc = (UA_NodeClass)((size_t)nc >> 1);
    }
    printf("%s %.*s\n", nodeClassNames[targetlevel], (int)pos, pathString);

    if(depth == 0)
        return;

    /* Read the attribute */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.nodeId = current;
    bd.referenceTypeId = UA_NS0ID(HIERARCHICALREFERENCES);
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME | UA_BROWSERESULTMASK_NODECLASS;

    UA_BrowseResult br = UA_Client_browse(client, NULL, 0, &bd);

    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_ReferenceDescription *rd = &br.references[i];
        if(!UA_ExpandedNodeId_isLocal(&rd->nodeId))
            continue;
        char browseName[80];
        int len = snprintf(browseName, 80, "%d:%.*s",
                           (unsigned)rd->browseName.namespaceIndex,
                           (int)rd->browseName.name.length,
                           (char*)rd->browseName.name.data);
        if(len < 0)
            continue;
        if(len > 80)
            len = 80;
        memcpy(pathString + pos, "/", 1);
        memcpy(pathString + pos + 1, browseName, len);
        exploreRecursive(pathString, pos + 1 + (size_t)len, rd->nodeId.nodeId, rd->nodeClass, depth-1);
    }

    UA_BrowseResult_clear(&br);
}

static void
explore(void) {
    /* Parse the arguments */
    char *pathArg = NULL;
    size_t depth = 20;

    for(; tokenPos < tokensSize; tokenPos++) {
        /* AttributeOperand */
        if(strncmp(tokens[tokenPos], "--", 2) != 0) {
            if(pathArg != NULL) {
                usage();
                return;
            }
            pathArg = tokens[tokenPos];
            continue;
        }

        /* Maximum depth */
        if(strcmp(tokens[tokenPos], "--depth") == 0) {
            tokenPos++;
            if(tokenPos == tokensSize) {
                usage();
                return;
            }
            depth = (size_t)atoi(tokens[tokenPos]);
            continue;
        }

        /* Unknown */
        usage();
        return;
    }

    /* Connect */
    connectClient();

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(pathArg));
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);

    /* Resolve the RelativePath */
    if(ao.browsePath.elementsSize > 0) {
        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.relativePath = ao.browsePath;
        bp.startingNode = ao.nodeId;

        UA_BrowsePathResult bpr =
            UA_Client_translateBrowsePathToNodeIds(client, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD)
            abortWithStatus(bpr.statusCode);

        /* Validate the response */
        if(bpr.targetsSize != 1) {
            abortWithMessage("The RelativePath did resolve to %u different NodeIds\n",
                             (unsigned)bpr.targetsSize);
            return;
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            abortWithMessage("The RelativePath was not fully resolved\n");
            return;
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            abortWithMessage("The RelativePath resolves to an ExpandedNodeId "
                             "on a different server\n");
            return;
        }

        UA_NodeId_clear(&ao.nodeId);
        ao.nodeId = bpr.targets[0].targetId.nodeId;
        UA_ExpandedNodeId_init(&bpr.targets[0].targetId);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Read the NodeClass of the root node */
    UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
    res = UA_Client_readNodeClassAttribute(client, ao.nodeId, &nc);
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);

    char relativepath[512];
    size_t pos = 0;
    if (pathArg) {
        pos = strlen(pathArg);
        memcpy(relativepath, pathArg, pos);
    }

    exploreRecursive(relativepath, pos, ao.nodeId, nc, depth);

    UA_AttributeOperand_clear(&ao);
}

/*****************/
/* Service Calls */
/*****************/

static void
processInputTokens(void) {
    if(tokensSize == 0)
        return;

    char *service = tokens[tokenPos++];
    if(strcmp(service, "getendpoints") == 0) {
        getEndpoints();
    } else if(strcmp(service, "read") == 0) {
        readService();
    } else if(strcmp(service, "browse") == 0) {
        browseService();
    } else if(strcmp(service, "write") == 0) {
        writeService();
    } else if(strcmp(service, "explore") == 0) {
        explore();
    } else {
        if(shellMode) {
            /* Quit the shell */
            if(strcmp(service, "q") == 0 ||
               strcmp(service, "quit") == 0 ||
               strcmp(service, "close") == 0) {
                shellMode = false;
                return;
            }
        }
        usage(); /* Unknown service */
    }
}

/******************/
/* Option Parsing */
/******************/

/* Parse options beginning with --.
 * Returns the position in the argv list. */
static int
parseOptions(int argc, char **argv, int argpos) {
    for(; argpos < argc; argpos++) {
        /* End of the arguments list */
        if(strncmp(argv[argpos], "--", 2) != 0)
            break;

        /* Help */
        if(strcmp(argv[argpos], "--help") == 0)
            usage();

        /* Username/Password */
        if(strcmp(argv[argpos], "--username") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            username = argv[argpos];
            continue;
        }
        if(strcmp(argv[argpos], "--password") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            password = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "--loglevel") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            logLevel = (UA_LogLevel)atoi(argv[argpos]);
            continue;
        }

        if(strcmp(argv[argpos], "--certificate") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            certificate = loadFile(argv[argpos]);
            continue;
        }

        if(strcmp(argv[argpos], "--privatekey") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            privateKey = loadFile(argv[argpos]);
            continue;
        }

        if(strcmp(argv[argpos], "--securitypolicy") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            securityPolicyUri = UA_STRING_ALLOC(argv[argpos]);
            continue;
        }

        /* Unknown option */
        usage();
    }

    return argpos;
}

static void
tokenize(char *line) {
    /* Quotes */
    bool in_single = false, in_double = false;

    /* If a token begins with [ or {, count opening
     * and closing braces to get a json object or array */
    unsigned braces = 0;

    tokensSize = 0;
    char *pos = line; /* For backslash-escaping, pos is the write-pos */
    char *begin = line;
    for(; (*pos = *line); pos++, line++) {
        /* Break tokens at spaces, skip repeated space */
        if(isspace(*line)) {
            if(!in_single && !in_double && braces == 0) {
                if(begin != line) {
                    *pos = '\0';
                    tokens[tokensSize++] = begin;
                }
                pos = line;
                begin = line + 1;
            }
            continue;
        }

        switch(*line) {
        /* Going in and out of strings */
        case '\'': if(!in_double) { in_single = !in_single; } break;
        case '"':  if(!in_single) { in_double = !in_double; } break;

        /* Backslash escaping outside of single-quotes.
         * Keep the backslash in the token. */
        case '\\': if(!in_single && line[1]) { *pos = *(++line); } break;

        /* Opening and closing braces */
        case '[':
        case '{':
            if(!in_double && !in_single && (braces > 0 || begin == line)) { braces++; } break;
        case ']':
        case '}':
            if(!in_double && !in_single && braces > 0) { braces--; } break;

        /* Normal character */
        default: break;
        }
    }

    /* Add the last token which ended the loop */
    if(begin != line)
        tokens[tokensSize++] = begin;
}

static void
shellInterface(void) {
    shellMode = true;

    char *line;
    while(shellMode && (line = readline(">>> ")) != NULL) {
        /* Lines that don't begin with a space are added to the history */
        if(*line && !isspace(*line))
            add_history(line);

        /* Tokenize the input and process */
        tokenPos = 0;
        tokenize(line);
        processInputTokens();

        /* Clean up and repeat */
        free(line);
    }
}

/****************/
/* Main Program */
/****************/

int
main(int argc, char **argv) {
    /* Read the command line options. Set used options to NULL.
     * Service-specific options are parsed later. */
    if(argc < 2)
        usage();

    /* Parse the options */
    int argpos = parseOptions(argc, argv, 1);

    /* Get the url */
    if(argpos >= argc)
        usage();
    url = argv[argpos++];

    /* Initialize the client config */
    cc.logging = &stderrLog;
    UA_ClientConfig_setDefault(&cc);

    /* TODO: Trustlist end revocation list */
#ifdef UA_ENABLE_ENCRYPTION
    if(certificate.length > 0) {
        UA_StatusCode res =
            UA_ClientConfig_setDefaultEncryption(&cc, certificate, privateKey,
                                                 NULL, 0, NULL, 0);
        if(res != UA_STATUSCODE_GOOD)
            exit(EXIT_FAILURE);
    }
#endif

    /* Accept all certificates without a trustlist */
    cc.certificateVerification.clear(&cc.certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc.certificateVerification);

    /* Filter endpoints with the securitypolicy.
     * The allocated string gets cleaned up as part of the client config. */
    cc.securityPolicyUri = securityPolicyUri;

    /* Initialize the client */
    client = UA_Client_newWithConfig(&cc);
    if(!client) {
        fprintf(stderr, "Client configuration invalid\n");
        exit(EXIT_FAILURE);
    }

    /* Too many arguments */
    if(argc - argpos > MAX_TOKENS)
        usage();

    if(argpos >= argc) {
        /* No service call -> Shell */
        shellInterface();
    } else {
        /* Move remaining arguments to the tokens */
        for(int i = argpos; i < argc; i++)
            tokens[i-argpos] = argv[i];
        tokensSize = (size_t)(argc - argpos);

        /* Process the tokens */
        tokenPos = 0;
        processInputTokens();
    }

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    UA_Client_delete(client);
    return return_value;
}
