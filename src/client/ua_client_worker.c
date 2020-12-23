/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client_internal.h"

static void
asyncServiceTimeoutCheck(UA_Client *client) {
    UA_DateTime now = UA_DateTime_nowMonotonic();

    /* Timeout occurs, remove the callback */
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        if(!ac->timeout)
           continue;

        if(ac->start + (UA_DateTime)(ac->timeout * UA_DATETIME_MSEC) <= now) {
            LIST_REMOVE(ac, pointers);
            UA_Client_AsyncService_cancel(client, ac, UA_STATUSCODE_BADTIMEOUT);
            UA_free(ac);
        }
    }
}

static void
backgroundConnectivityCallback(UA_Client *client, void *userdata,
                               UA_UInt32 requestId, const UA_ReadResponse *response) {
    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        if (client->config.inactivityCallback)
            client->config.inactivityCallback(client);
    }
    client->pendingConnectivityCheck = false;
    client->lastConnectivityCheck = UA_DateTime_nowMonotonic();
}

static UA_StatusCode
UA_Client_backgroundConnectivity(UA_Client *client) {
    if(!client->config.connectivityCheckInterval)
        return UA_STATUSCODE_GOOD;

    if (client->pendingConnectivityCheck)
        return UA_STATUSCODE_GOOD;

    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextDate = client->lastConnectivityCheck + (UA_DateTime)(client->config.connectivityCheckInterval * UA_DATETIME_MSEC);

    if(now <= nextDate)
        return UA_STATUSCODE_GOOD;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);

    request.nodesToRead = &rvid;
    request.nodesToReadSize = 1;

    UA_StatusCode retval = __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                                                    (UA_ClientAsyncServiceCallback)backgroundConnectivityCallback,
                                                    &UA_TYPES[UA_TYPES_READRESPONSE], NULL, NULL);

    client->pendingConnectivityCheck = true;

    return retval;
}

/**
 * Main Client Loop
 * ----------------
 * Start: Spin up the workers and the network layer
 * Iterate: Process repeated callbacks and events in the network layer.
 *          This part can be driven from an external main-loop in an
 *          event-driven single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer,
 *       clean up */

static void
clientExecuteRepeatedCallback(UA_Client *client, UA_ApplicationCallback cb,
                              void *callbackApplication, void *data) {
    cb(callbackApplication, data);
    /* TODO: Use workers in the client
     * UA_WorkQueue_enqueue(&client->workQueue, cb, callbackApplication, data); */
}

UA_StatusCode UA_Client_run_iterate(UA_Client *client, UA_UInt16 timeout) {
// TODO connectivity check & timeout features for the async implementation (timeout == 0)
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_StatusCode retvalPublish = UA_Client_Subscriptions_backgroundPublish(client);
    if(client->state >= UA_CLIENTSTATE_SESSION && retvalPublish != UA_STATUSCODE_GOOD)
        return retvalPublish;
#endif
    /* Make sure we have an open channel */

    /************************************************************/
    /* FIXME: This is a dirty workaround */
    if(client->state >= UA_CLIENTSTATE_SECURECHANNEL)
        retval = openSecureChannel(client, true);
    /* FIXME: Will most likely break somewhere in the future */
    /************************************************************/

    if(timeout) {
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        retval = UA_Client_backgroundConnectivity(client);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        UA_DateTime maxDate = UA_DateTime_nowMonotonic() + (timeout * UA_DATETIME_MSEC);
        retval = receiveServiceResponse(client, NULL, NULL, maxDate, NULL);
        if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
            retval = UA_STATUSCODE_GOOD;
    } else {
        UA_DateTime now = UA_DateTime_nowMonotonic();
        UA_Timer_process(&client->timer, now,
                         (UA_TimerExecutionCallback)clientExecuteRepeatedCallback, client);

        UA_ClientState cs = UA_Client_getState(client);
        retval = UA_Client_connect_iterate(client);

        /* Connection failed, drop the rest */
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        if((cs == UA_CLIENTSTATE_SECURECHANNEL) || (cs == UA_CLIENTSTATE_SESSION)) {
            /* Check for new data */
            retval = receiveServiceResponseAsync(client, NULL, NULL);
        } else {
            retval = receivePacketAsync(client);
        }
    }
#ifdef UA_ENABLE_SUBSCRIPTIONS
        /* The inactivity check must be done after receiveServiceResponse*/
        UA_Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif
        asyncServiceTimeoutCheck(client);

#ifndef UA_ENABLE_MULTITHREADING
        /* Process delayed callbacks when all callbacks and network events are
         * done */
        UA_WorkQueue_manuallyProcessDelayed(&client->workQueue);
#endif
    return retval;
}
