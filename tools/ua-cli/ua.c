/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/plugin/log.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>

#include <stdio.h>
#include <ctype.h>

static UA_Client *client = NULL;
static UA_ClientConfig cc;
static UA_NodeId nodeidval = {0};
static char *url = NULL;
static char *service = NULL;
static char *nodeid = NULL;
static char *value = NULL;
static char *username = NULL;
static char *password = NULL;
static UA_UInt32 attr = UA_ATTRIBUTEID_VALUE;
#ifdef UA_ENABLE_JSON_ENCODING
static UA_Boolean json = false;
#endif

/* Custom logger that prints to stderr. So the "good output" can be easily separated. */
UA_LogLevel logLevel = UA_LOGLEVEL_INFO;

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
    if(logLevel > level)
        return;

    /* Set to fatal if the level is outside the range */
    int logLevelSlot = ((int)level / 100) - 1;
    if(logLevelSlot < 0 || logLevelSlot > 5)
        logLevelSlot = 5;

    /* Log */
#define LOGBUFSIZE 512
    UA_Byte logbuf[LOGBUFSIZE];
    UA_String out = {LOGBUFSIZE, logbuf};
    UA_String_vprintf(&out, msg, args);
    fprintf(stderr, "%s/%s" ANSI_COLOR_RESET "\t",
           logLevelNames[logLevelSlot], logCategoryNames[category]);
    fprintf(stderr, "%s\n", logbuf);
    fflush(stderr);
}

UA_Logger stderrLog = {cliLog, NULL, NULL};

static void
usage(void) {
    fprintf(stderr, "Usage: ua <server-url> <service> [--json] [--help]\n"
            " <server-url>: opc.tcp://domain[:port]\n"
            " <service> -> getendpoints: Log the endpoint descriptions of the server\n"
            " <service> -> read <nodeid>: Read an attribute of the node\n"
            "   --attr <attribute-id | attribute-name>: Attribute to read from the node. "
            "[default: value]\n"
            //" <service> -> browse <nodeid>: Browse the Node\n"
            //" <service> -> call <method-id> <object-id> <arguments>: Call the method \n"
            //" <service> -> write <nodeid> <value>: Write an attribute of the node\n"
#ifdef UA_ENABLE_JSON_ENCODING
            " --json: Format output as JSON\n"
#endif
            " --username: Username for the session creation\n"
            " --password: Password for the session creation\n"
            " --help: Print this message\n");
    exit(EXIT_FAILURE);
}

static void
printType(void *p, const UA_DataType *type) {
    UA_ByteString out = UA_BYTESTRING_NULL;
#ifdef UA_ENABLE_JSON_ENCODING
    if(!json) {
        UA_print(p, type, &out);
    } else {
        UA_StatusCode res = UA_encodeJson(p, type, &out, NULL);
        (void)res;
    }
#else
    UA_print(p, type, &out);
#endif
    printf("%.*s\n", (int)out.length, out.data);
    UA_ByteString_clear(&out);
}

static void
abortWithStatus(UA_StatusCode res) {
    fprintf(stderr, "Aborting with status code %s\n", UA_StatusCode_name(res));
    exit(res);
}

static void
getEndpoints(int argc, char **argv) {
    /* Get endpoints */
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
parseNodeId(void) {
    UA_StatusCode res = UA_NodeId_parse(&nodeidval, UA_STRING(nodeid));
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Could not parse the NodeId\n");
        exit(EXIT_FAILURE);
    }
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
readAttr(int argc, char **argv) {
    parseNodeId();
    connectClient();

    /* Read */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = nodeidval;
    rvid.attributeId = attr;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    req.nodesToReadSize = 1;
    req.nodesToRead = &rvid;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);

    /* Print the result */
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       resp.resultsSize != 1)
        resp.responseHeader.serviceResult = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        printType(&resp.results[0], &UA_TYPES[UA_TYPES_DATAVALUE]);
    else
        abortWithStatus(resp.responseHeader.serviceResult);

    UA_ReadResponse_clear(&resp);
    UA_NodeId_clear(&nodeidval);
}

static const char *attributeIds[27] = {
    "nodeid", "nodeclass", "browsename", "displayname", "description",
    "writemask", "userwritemask", "isabstract", "symmetric", "inversename",
    "containsnoloops", "eventnotifier", "value", "datatype", "valuerank",
    "arraydimensions", "accesslevel", "useraccesslevel",
    "minimumsamplinginterval", "historizing", "executable", "userexecutable",
    "datatypedefinition", "rolepermissions", "userrolepermissions",
    "accessrestrictions", "accesslevelex"
};

/* Parse options beginning with --.
 * Returns the position in the argv list. */
static int
parseOptions(int argc, char **argv, int argpos) {
    for(; argpos < argc; argpos++) {
        /* End of the arguments list */
        if(strncmp(argv[argpos], "--", 2) == 0)
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

        /* Parse attribute to be read or written */
        if(strcmp(argv[argpos], "--attr") == 0) {
            argpos++;
            if(argpos == argc)
                usage();

            /* Try to parse integer attr argument */
            attr = (UA_UInt32)atoi(argv[argpos]);
            if(attr != 0)
                continue;

            /* Convert to lower case and try to find in table */
            for(char *w = argv[argpos]; *w; w++)
                *w = (char)tolower(*w);
            for(UA_UInt32 i = 0; i < 26; i++) {
                if(strcmp(argv[argpos], attributeIds[i]) == 0) {
                    attr = i+1;
                    break;
                }
            }
            if(attr != 0)
                continue;
            usage();
        }

        /* Output JSON format */
#ifdef UA_ENABLE_JSON_ENCODING
        if(strcmp(argv[argpos], "--json") == 0) {
            json = true;
            continue;
        }
#endif

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

    /* Get the url and service. Must not be a -- option string */
    if(strstr(argv[1], "--") == argv[1] ||
       strstr(argv[2], "--") == argv[2])
        usage();

    url = argv[1];
    service = argv[2];

    /* If not a -- option string, then this is the NodeId */
    int argpos = 3;
    if(argc > argpos && strstr(argv[argpos], "--") != argv[argpos]) {
        nodeid = argv[argpos];
        argv[argpos] = NULL;
        argpos++;

        /* If not a -- option string, then this is the value */
        if(argc > argpos && strstr(argv[argpos], "--") != argv[argpos]) {
            value = argv[argpos];
            argv[argpos] = NULL;
            argpos++;
        }
    }

    /* Initialize the client config */
    cc.logging = &stderrLog;
    UA_ClientConfig_setDefault(&cc);

    /* Parse the options */
    argpos = parseOptions(argc, argv, argpos);
    if(argpos < argc - 1)
        usage(); /* Not all options have been parsed */

    /* Initialize the client */
    client = UA_Client_newWithConfig(&cc);
    if(!client) {
        fprintf(stderr, "Client configuration invalid\n");
        exit(EXIT_FAILURE);
    }

    /* Execute the service */
    if(strcmp(service, "getendpoints") == 0) {
        if(!nodeid && !value)
            getEndpoints(argc, argv);
    } else if(strcmp(service, "read") == 0) {
        if(nodeid && !value)
            readAttr(argc, argv);
    } else {
        usage(); /* Unknown service */
    }
    //else if(strcmp(service, "browse") == 0) {
    //    if(nodeid && !value)
    //        return browse(argc, argv);
    //}
    //else if(strcmp(argv[1], "write") == 0) {
    //    if(nodeid && value)
    //        return writeAttr(argc-argpos, &argv[argpos]);
    //}

    UA_Client_delete(client);
    return 0;
}
