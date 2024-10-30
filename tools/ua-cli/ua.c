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

#include <stdio.h>

static UA_Client *client = NULL;
static UA_ClientConfig cc;
static char *url = NULL;
static char *service = NULL;
static char *username = NULL;
static char *password = NULL;
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
            " <service> -> read <AttributeOperand>: Read an attribute\n"
            " <service> -> browse <AttributeOperand>: Browse the references to and from the node\n"
            //" <service> -> call <method-id> <object-id> <arguments>: Call the method \n"
            //" <service> -> write <nodeid> <value>: Write an attribute of the node\n"
            " Options:\n"
            " --username: Username for the session creation\n"
            " --password: Password for the session creation\n"
            " --loglevel: Logging detail [1 -> TRACE, 6 -> FATAL]\n"
            " --help: Print this message\n");
    exit(EXIT_FAILURE);
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
read(int argc, char **argv, int argpos) {
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
browse(int argc, char **argv, int argpos) {
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
        read(argc, argv, argpos);
    } else if(strcmp(service, "browse") == 0) {
        browse(argc, argv, argpos);
    } else {
        usage(); /* Unknown service */
    }
    //else if(strcmp(argv[1], "write") == 0) {
    //    if(nodeid && value)
    //        return writeAttr(argc-argpos, &argv[argpos]);
    //}

    UA_Client_delete(client);
    return return_value;
}
