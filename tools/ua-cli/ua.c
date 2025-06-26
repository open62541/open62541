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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static UA_Client *client = NULL;
static UA_ClientConfig cc;
static char *url = NULL;
static char *service = NULL;
static char *username = NULL;
static char *password = NULL;
static UA_ByteString certificate;
static UA_ByteString privateKey;
static UA_ByteString securityPolicyUri;
int return_value = 0;

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

static void
usage(void) {
    fprintf(stderr, "Usage: ua [options] [--help] <server-url> <service>\n"
            " <server-url>: opc.tcp://domain[:port]\n"
            " <service> -> getendpoints: Print the endpoint descriptions of the server\n"
            " <service> -> read   <AttributeOperand>: Read an attribute\n"
            " <service> -> browse <AttributeOperand>: Browse the references of a node\n"
            " <service> -> write  <AttributeOperand> <value>: Write an attribute\n"
            " <service> -> explore <RelativePath> [--depth <int>]: Print the structure of the information model below the indicated node\n"
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

static void
abortWithStatus(UA_StatusCode res) {
    fprintf(stderr, "Aborting with status code %s\n", UA_StatusCode_name(res));
    if(client) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
    exit(res);
}

static void
connectClient(void) {
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

static void
getEndpoints(int argc, char **argv, int argpos) {
    /* Validate the arguments */
    if(argpos != argc) {
        fprintf(stderr, "Arguments after \"getendpoints\" could not be parsed\n");
        exit(EXIT_FAILURE);
    }

    /* Get Endpoints */
    size_t endpointDescriptionsSize = 0;
    UA_EndpointDescription* endpointDescriptions = NULL;
    UA_StatusCode res = UA_Client_getEndpoints(client, url,
                                               &endpointDescriptionsSize,
                                               &endpointDescriptions);
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);

    /* Print the results */
    UA_Variant var;
    UA_Variant_setArray(&var, endpointDescriptions, endpointDescriptionsSize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    printType(&var, &UA_TYPES[UA_TYPES_VARIANT]);

    /* Delete the allocated array */
    UA_Variant_clear(&var);
}

static void
readService(int argc, char **argv, int argpos) {
    /* Validate the arguments */
    if(argpos != argc - 1) {
        fprintf(stderr, "The read service takes an AttributeOperand "
                "expression as the last argument\n");
        exit(EXIT_FAILURE);
    }

    /* Connect */
    connectClient();

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(argv[argpos]));
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
            fprintf(stderr, "The RelativePath did resolve to %u different NodeIds\n",
                    (unsigned)bpr.targetsSize);
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            fprintf(stderr, "The RelativePath was not fully resolved\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            fprintf(stderr, "The RelativePath resolves to an ExpandedNodeId "
                    "on a different server\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
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
writeService(int argc, char **argv, int argpos) {
    /* Validate the arguments */
    if(argpos + 1 >= argc) {
        fprintf(stderr, "The Write Service takes an AttributeOperand "
                "expression and the value as arguments\n");
        exit(EXIT_FAILURE);
    }

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(argv[argpos++]));
    if(res != UA_STATUSCODE_GOOD)
        abortWithStatus(res);

    /* Aggregate all the remaining arguments and parse them as JSON */
    UA_String valstr = UA_STRING_NULL;
    for(; argpos < argc; argpos++) {
        UA_String_append(&valstr, UA_STRING(argv[argpos]));
        if(argpos != argc - 1)
            UA_String_append(&valstr, UA_STRING(" "));
    }

    if(valstr.length == 0) {
        fprintf(stderr, "No value defined\n");
        exit(EXIT_FAILURE);
    }

    /* Detect a few basic "naked" datatypes.
     * Otherwise try to decode a variant */
    UA_Boolean b;
    UA_Int32 i;
    UA_Float ff;
    UA_Variant v;
    UA_String s = UA_STRING_NULL;
    UA_String f = UA_STRING("false");
    UA_String t = UA_STRING("true");
    if(UA_String_equal(&valstr, &f)) {
        b = false;
        UA_Variant_setScalar(&v, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
        v.storageType = UA_VARIANT_DATA_NODELETE;
    } else if(UA_String_equal(&valstr, &t)) {
        b = true;
        UA_Variant_setScalar(&v, &b, &UA_TYPES[UA_TYPES_BOOLEAN]);
        v.storageType = UA_VARIANT_DATA_NODELETE;
    } else if(valstr.data[0] == '\"') {
        res = UA_decodeJson(&valstr, &s, &UA_TYPES[UA_TYPES_STRING], NULL);
        UA_Variant_setScalar(&v, &s, &UA_TYPES[UA_TYPES_STRING]);
        v.storageType = UA_VARIANT_DATA_NODELETE;
    } else if(valstr.data[0] >= '0' && valstr.data[0] <= '9') {
        res = UA_decodeJson(&valstr, &i, &UA_TYPES[UA_TYPES_INT32], NULL);
        UA_Variant_setScalar(&v, &i, &UA_TYPES[UA_TYPES_INT32]);
        if(res != UA_STATUSCODE_GOOD) {
            res = UA_decodeJson(&valstr, &ff, &UA_TYPES[UA_TYPES_FLOAT], NULL);
            UA_Variant_setScalar(&v, &ff, &UA_TYPES[UA_TYPES_FLOAT]);
        }
        v.storageType = UA_VARIANT_DATA_NODELETE;
    } else if(valstr.data[0] == '{') {
        /* JSON Variant */
        res = UA_decodeJson(&valstr, &v, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    } else if(valstr.data[0] == '(') {
        /* Data type name in parentheses */
        UA_STACKARRAY(char, type, valstr.length);
        int elem = sscanf((char*)valstr.data, "(%[^)])", type);
        if(elem <= 0) {
            fprintf(stderr, "Wrong datatype definition\n");
            exit(EXIT_FAILURE);
        }

        /* Find type under the name */
        const UA_DataType *datatype = NULL;
        for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
            if(strcmp(UA_TYPES[i].typeName, type) == 0) {
                datatype = &UA_TYPES[i];
                break;
            }
        }

        if(!datatype) {
            fprintf(stderr, "Data type %s unknown\n", type);
            exit(EXIT_FAILURE);
        }

        valstr.data += 2 + strlen(type);
        valstr.length -= 2 + strlen(type);

        /* Parse */
        void *val = UA_new(datatype);
        res = UA_decodeJson(&valstr, val, datatype, NULL);
        UA_Variant_setScalar(&v, val, datatype);
    } else {
        res = UA_STATUSCODE_BADDECODINGERROR;
    }

    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Could not parse the value\n");
        exit(EXIT_FAILURE);
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
            fprintf(stderr, "The RelativePath did resolve to %u different NodeIds\n",
                    (unsigned)bpr.targetsSize);
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            fprintf(stderr, "The RelativePath was not fully resolved\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            fprintf(stderr, "The RelativePath resolves to an ExpandedNodeId "
                    "on a different server\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
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
    UA_String_clear(&s);
}

static void
browseService(int argc, char **argv, int argpos) {
    /* Validate the arguments */
    if(argpos != argc - 1) {
        fprintf(stderr, "The browse service takes an AttributeOperand "
                "expression as the last argument\n");
        exit(EXIT_FAILURE);
    }

    /* Connect */
    connectClient();

    /* Parse the AttributeOperand */
    UA_AttributeOperand ao;
    UA_StatusCode res = UA_AttributeOperand_parse(&ao, UA_STRING(argv[argpos]));
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
            fprintf(stderr, "The RelativePath did resolve to %u different NodeIds\n",
                    (unsigned)bpr.targetsSize);
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            fprintf(stderr, "The RelativePath was not fully resolved\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            fprintf(stderr, "The RelativePath resolves to an ExpandedNodeId "
                    "on a different server\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
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
explore(int argc, char **argv, int argpos) {
    /* Parse the arguments */
    char *pathArg = NULL;
    size_t depth = 20;

    for(; argpos < argc; argpos++) {
        /* AttributeOperand */
        if(strncmp(argv[argpos], "--", 2) != 0) {
            if(pathArg != NULL)
                usage();
            pathArg = argv[argpos];
            continue;
        }

        /* Maximum depth */
        if(strcmp(argv[argpos], "--depth") == 0) {
            argpos++;
            if(argpos == argc)
                usage();
            depth = (size_t)atoi(argv[argpos]);
            continue;
        }

        /* Unknown */
        usage();
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
            fprintf(stderr, "The RelativePath did resolve to %u different NodeIds\n",
                    (unsigned)bpr.targetsSize);
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(bpr.targets[0].remainingPathIndex != UA_UINT32_MAX) {
            fprintf(stderr, "The RelativePath was not fully resolved\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
        }

        if(!UA_ExpandedNodeId_isLocal(&bpr.targets[0].targetId)) {
            fprintf(stderr, "The RelativePath resolves to an ExpandedNodeId "
                    "on a different server\n");
            abortWithStatus(UA_STATUSCODE_BADINTERNALERROR);
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

int
main(int argc, char **argv) {
    /* Read the command line options. Set used options to NULL.
     * Service-specific options are parsed later. */
    if(argc < 3)
        usage();

    /* Parse the options */
    int argpos = parseOptions(argc, argv, 1);
    if(argpos > argc - 2)
        usage();
    url = argv[argpos++];
    service = argv[argpos++];

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

    /* Execute the service */
    if(strcmp(service, "getendpoints") == 0) {
        getEndpoints(argc, argv, argpos);
    } else if(strcmp(service, "read") == 0) {
        readService(argc, argv, argpos);
    } else if(strcmp(service, "browse") == 0) {
        browseService(argc, argv, argpos);
    } else if(strcmp(service, "write") == 0) {
        writeService(argc, argv, argpos);
    } else if(strcmp(service, "explore") == 0) {
        explore(argc, argv, argpos);
    } else {
        usage(); /* Unknown service */
    }

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    UA_Client_delete(client);
    return return_value;
}
