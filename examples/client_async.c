/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <stdio.h>
#include "open62541.h"

#ifdef _WIN32
# include <windows.h>
# define UA_sleep_ms(X) Sleep(X)
#else
# include <unistd.h>
# define UA_sleep_ms(X) usleep(X * 1000)
#endif

#define NODES_EXIST
/* async connection callback, it only gets called after the completion of the whole
 * connection process*/
static void
onConnect (UA_Client *client, void *userdata, UA_UInt32 requestId,
           void *status) {
    printf ("Async connect returned with status code %s\n",
            UA_StatusCode_name (*(UA_StatusCode *) status));
}

static
void
fileBrowsed (UA_Client *client, void *userdata, UA_UInt32 requestId,
             UA_BrowseResponse *response) {
    printf ("%-50s%i\n", "Received BrowseResponse for request ", requestId);
    UA_String us = *(UA_String *) userdata;
    printf ("---%.*s passed safely \n", (int) us.length, us.data);
}

/*high-level function callbacks*/
static
void
readValueAttributeCallback (UA_Client *client, void *userdata,
                            UA_UInt32 requestId, UA_Variant *var) {
    printf ("%-50s%i\n", "Read value attribute for request", requestId);

    if (UA_Variant_hasScalarType (var, &UA_TYPES[UA_TYPES_INT32])) {
        UA_Int32 int_val = *(UA_Int32*) var->data;
        printf ("---%-40s%-8i\n",
                "Reading the value of node (1, \"the.answer\"):", int_val);
    }

    /*more type distinctions possible*/
    return;
}

static
void
attrWritten (UA_Client *client, void *userdata, UA_UInt32 requestId,
             UA_WriteResponse *response) {
    /*assuming no data to be retrieved by writing attributes*/
    printf ("%-50s%i\n", "Wrote value attribute for request ", requestId);
    UA_WriteResponse_deleteMembers(response);
}

#ifdef NODES_EXIST
static void
methodCalled (UA_Client *client, void *userdata, UA_UInt32 requestId,
              UA_CallResponse *response) {

    printf ("%-50s%i\n", "Called method for request ", requestId);
    size_t outputSize;
    UA_Variant *output;
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if (retval == UA_STATUSCODE_GOOD) {
        if (response->resultsSize == 1)
            retval = response->results[0].statusCode;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if (retval != UA_STATUSCODE_GOOD) {
        UA_CallResponse_deleteMembers (response);
    }

    /* Move the output arguments */
    output = response->results[0].outputArguments;
    outputSize = response->results[0].outputArgumentsSize;
    response->results[0].outputArguments = NULL;
    response->results[0].outputArgumentsSize = 0;

    if (retval == UA_STATUSCODE_GOOD) {
        printf ("---Method call was successful, returned %lu values.\n",
                (unsigned long) outputSize);
        UA_Array_delete (output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    }
    else {
        printf ("---Method call was unsuccessful, returned %x values.\n",
                retval);
    }
    UA_CallResponse_deleteMembers (response);
}

static void
translateCalled (UA_Client *client, void *userdata, UA_UInt32 requestId,
                 UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    printf ("%-50s%i\n", "Translated path for request ", requestId);

    if (response->results[0].targetsSize == 1) {
        return;
    }
    UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers (response);
}
#endif

int
main (int argc, char *argv[]) {
    UA_Client *client = UA_Client_new (UA_ClientConfig_default);
    UA_UInt32 reqId = 0;
    UA_String userdata = UA_STRING ("userdata");

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init (&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new ();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC (0,
    UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */

    UA_Client_connect_async (client, "opc.tcp://localhost:4840", onConnect,
                             NULL);

    /*Windows needs time to response*/
    UA_sleep_ms(100);

    /* What happens if client tries to send request before connected? */
    UA_Client_sendAsyncBrowseRequest (client, &bReq, fileBrowsed, &userdata,
                                      &reqId);

    UA_DateTime startTime = UA_DateTime_nowMonotonic();
    do {
        /*TODO: fix memory-related bugs if condition not checked*/
        if (UA_Client_getState (client) == UA_CLIENTSTATE_SESSION) {
            /* If not connected requests are not sent */
            UA_Client_sendAsyncBrowseRequest (client, &bReq, fileBrowsed,
                                              &userdata, &reqId);
        }
        /* Requests are processed */
        UA_BrowseRequest_deleteMembers(&bReq);
        UA_Client_run_iterate (client, 0);
        UA_sleep_ms(100);

        /* Break loop if server cannot be connected within 2s -- prevents build timeout */
        if (UA_DateTime_nowMonotonic() - startTime > 2000 * UA_DATETIME_MSEC)
            break;
    }
    while (reqId < 10);

    /* Demo: high-level functions */
    UA_Int32 value = 0;
    UA_Variant myVariant;
    UA_Variant_init(&myVariant);

    UA_Variant input;
    UA_Variant_init (&input);

    for (UA_UInt16 i = 0; i < 5; i++) {
        if (UA_Client_getState (client) == UA_CLIENTSTATE_SESSION) {
            /* writing and reading value 1 to 5 */
            UA_Variant_setScalarCopy (&myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
            value++;
            UA_Client_writeValueAttribute_async(client,
                                                UA_NODEID_STRING (1, "the.answer"),
                                                &myVariant, attrWritten, NULL,
                                                &reqId);
            UA_Variant_deleteMembers (&myVariant);

            UA_Client_readValueAttribute_async(client,
                                               UA_NODEID_STRING (1, "the.answer"),
                                               readValueAttributeCallback, NULL,
                                               &reqId);

//TODO: check the existance of the nodes inside these functions (otherwise seg faults)
#ifdef NODES_EXIST
            UA_String stringValue = UA_String_fromChars ("World");
            UA_Variant_setScalar (&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

            UA_Client_call_async(client,
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                 UA_NODEID_NUMERIC (1, 62541), 1, &input,
                                 methodCalled, NULL, &reqId);
            UA_String_deleteMembers(&stringValue);

    #define pathSize 3
            char *paths[pathSize] = { "Server", "ServerStatus", "State" };
            UA_UInt32 ids[pathSize] = { UA_NS0ID_ORGANIZES,
            UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASCOMPONENT };

            UA_Cient_translateBrowsePathsToNodeIds_async (client, paths, ids,
                                                          pathSize,
                                                          translateCalled, NULL,
                                                          &reqId);
#endif
            /* How often UA_Client_run_iterate is called depends on the number of request sent */
            UA_Client_run_iterate(client, 0);
            UA_Client_run_iterate(client, 0);
        }
    }
    UA_Client_run_iterate (client, 0);

    /* Async disconnect kills unprocessed requests */
    // UA_Client_disconnect_async (client, &reqId); //can only be used when connected = true
    // UA_Client_run_iterate (client, &timedOut);
    UA_Client_disconnect(client);
    UA_Client_delete (client);

    return (int) UA_STATUSCODE_GOOD;
}
