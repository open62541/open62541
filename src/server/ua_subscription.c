#include "ua_subscription.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_services.h"
#include "ua_nodestore.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

/*****************/
/* MonitoredItem */
/*****************/

UA_MonitoredItem * UA_MonitoredItem_new() {
    UA_MonitoredItem *new = UA_malloc(sizeof(UA_MonitoredItem));
    new->subscription = NULL;
    new->currentQueueSize = 0;
    new->maxQueueSize = 0;
    new->monitoredItemType = UA_MONITOREDITEMTYPE_CHANGENOTIFY; /* currently hardcoded */
    new->timestampsToReturn = UA_TIMESTAMPSTORETURN_SOURCE;
    UA_String_init(&new->indexRange);
    TAILQ_INIT(&new->queue);
    UA_NodeId_init(&new->monitoredNodeId);
    new->lastSampledValue = UA_BYTESTRING_NULL;
    memset(&new->sampleJobGuid, 0, sizeof(UA_Guid));
    new->sampleJobIsRegistered = false;
    new->itemId = 0;
    return new;
}

void MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    MonitoredItem_unregisterSampleJob(server, monitoredItem);
    /* clear the queued samples */
    MonitoredItem_queuedValue *val, *val_tmp;
    TAILQ_FOREACH_SAFE(val, &monitoredItem->queue, listEntry, val_tmp) {
        TAILQ_REMOVE(&monitoredItem->queue, val, listEntry);
        UA_DataValue_deleteMembers(&val->value);
        UA_free(val);
    }
    monitoredItem->currentQueueSize = 0;
    LIST_REMOVE(monitoredItem, listEntry);
    UA_String_deleteMembers(&monitoredItem->indexRange);
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    UA_NodeId_deleteMembers(&monitoredItem->monitoredNodeId);
    UA_free(monitoredItem);
}

void UA_MoniteredItem_SampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_Subscription *sub = monitoredItem->subscription;
    if(monitoredItem->monitoredItemType != UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                             "Subscription %u | MonitoredItem %i | "
                             "Cannot process a monitoreditem that is not "
                             "a data change notification",
                             sub->subscriptionID, monitoredItem->itemId);
        return;
    }

    MonitoredItem_queuedValue *newvalue = UA_malloc(sizeof(MonitoredItem_queuedValue));
    if(!newvalue) {
        UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                               "Subscription %u | MonitoredItem %i | "
                               "Skipped a sample due to lack of memory",
                               sub->subscriptionID, monitoredItem->itemId);
        return;
    }
    UA_DataValue_init(&newvalue->value);
    newvalue->clientHandle = monitoredItem->clientHandle;

    /* Adjust timestampstoreturn to get source timestamp for triggering */
    UA_TimestampsToReturn ts = monitoredItem->timestampsToReturn;
    if(ts == UA_TIMESTAMPSTORETURN_SERVER)
        ts = UA_TIMESTAMPSTORETURN_BOTH;
    else if(ts == UA_TIMESTAMPSTORETURN_NEITHER)
        ts = UA_TIMESTAMPSTORETURN_SOURCE;

    /* Read the value */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = monitoredItem->monitoredNodeId;
    rvid.attributeId = monitoredItem->attributeID;
    rvid.indexRange = monitoredItem->indexRange;
    Service_Read_single(server, sub->session, ts, &rvid, &newvalue->value);

    /* Apply Filter */
    UA_Boolean hasValue = newvalue->value.hasValue;
    UA_Boolean hasServerTimestamp = newvalue->value.hasServerTimestamp;
    UA_Boolean hasServerPicoseconds = newvalue->value.hasServerPicoseconds;
    UA_Boolean hasSourceTimestamp = newvalue->value.hasSourceTimestamp;
    UA_Boolean hasSourcePicoseconds = newvalue->value.hasSourcePicoseconds;
    newvalue->value.hasServerTimestamp = false;
    newvalue->value.hasServerPicoseconds = false;
    if(monitoredItem->trigger == UA_DATACHANGETRIGGER_STATUS)
        newvalue->value.hasValue = false;
    if(monitoredItem->trigger < UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        newvalue->value.hasSourceTimestamp = false;
        newvalue->value.hasSourcePicoseconds = false;
    }

    /* Encode the data for comparison */
    size_t binsize = UA_calcSizeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_ByteString newValueAsByteString;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&newValueAsByteString, binsize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }
    size_t encodingOffset = 0;
    retval = UA_encodeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_DATAVALUE],
                             NULL, NULL, &newValueAsByteString, &encodingOffset);

    /* Restore the settings changed for the filter */
    newvalue->value.hasValue = hasValue;
    newvalue->value.hasServerTimestamp = hasServerTimestamp;
    newvalue->value.hasServerPicoseconds = hasServerPicoseconds;
    if(monitoredItem->timestampsToReturn == UA_TIMESTAMPSTORETURN_SERVER ||
       monitoredItem->timestampsToReturn == UA_TIMESTAMPSTORETURN_NEITHER) {
        newvalue->value.hasSourceTimestamp = false;
        newvalue->value.hasSourcePicoseconds = false;
    } else {
        newvalue->value.hasSourceTimestamp = hasSourceTimestamp;
        newvalue->value.hasSourcePicoseconds = hasSourcePicoseconds;
    }

    /* Error or the value has not changed */
    if(retval != UA_STATUSCODE_GOOD ||
       (monitoredItem->lastSampledValue.data &&
        UA_String_equal(&newValueAsByteString, &monitoredItem->lastSampledValue))) {
        UA_LOG_TRACE_SESSION(server->config.logger, sub->session, "Subscription %u | "
                             "MonitoredItem %u | Do not sample an unchanged value",
                             sub->subscriptionID, monitoredItem->itemId);
        goto cleanup;
    }

    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | MonitoredItem %u | "
                         "Sampling the value", sub->subscriptionID, monitoredItem->itemId);

    /* Is enough space in the queue? */
    if(monitoredItem->currentQueueSize >= monitoredItem->maxQueueSize) {
        MonitoredItem_queuedValue *queueItem;
        if(monitoredItem->discardOldest)
            queueItem = TAILQ_FIRST(&monitoredItem->queue);
        else
            queueItem = TAILQ_LAST(&monitoredItem->queue, QueueOfQueueDataValues);

        if(!queueItem) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session, "Subscription %u | "
                                   "MonitoredItem %u | Cannot remove an element from the full "
                                   "queue. Internal error!", sub->subscriptionID,
                                   monitoredItem->itemId);
            goto cleanup;
        }

        TAILQ_REMOVE(&monitoredItem->queue, queueItem, listEntry);
        UA_DataValue_deleteMembers(&queueItem->value);
        UA_free(queueItem);
        monitoredItem->currentQueueSize--;
    }

    /* If the read request returned a datavalue pointing into the nodestore, we
     * must make a deep copy to keep the datavalue across mainloop iterations */
    if(newvalue->value.hasValue &&
       newvalue->value.value.storageType == UA_VARIANT_DATA_NODELETE) {
        UA_Variant tempv = newvalue->value.value;
        UA_Variant_copy(&tempv, &newvalue->value.value);
    }

    /* Replace the comparison bytestring with the current sample */
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    monitoredItem->lastSampledValue = newValueAsByteString;

    /* Add the sample to the queue for publication */
    TAILQ_INSERT_TAIL(&monitoredItem->queue, newvalue, listEntry);
    monitoredItem->currentQueueSize++;
    return;

 cleanup:
    UA_ByteString_deleteMembers(&newValueAsByteString);
    UA_DataValue_deleteMembers(&newvalue->value);
    UA_free(newvalue);
}

UA_StatusCode
MonitoredItem_registerSampleJob(UA_Server *server, UA_MonitoredItem *mon) {
    UA_Job job;
    job.type = UA_JOBTYPE_METHODCALL;
    job.job.methodCall.method = (UA_ServerCallback)UA_MoniteredItem_SampleCallback;
    job.job.methodCall.data = mon;
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, job,
                                                    (UA_UInt32)mon->samplingInterval,
                                                    &mon->sampleJobGuid);
    if(retval == UA_STATUSCODE_GOOD)
        mon->sampleJobIsRegistered = true;
    return retval;
}

UA_StatusCode MonitoredItem_unregisterSampleJob(UA_Server *server, UA_MonitoredItem *mon) {
    if(!mon->sampleJobIsRegistered)
        return UA_STATUSCODE_GOOD;
    mon->sampleJobIsRegistered = false;
    return UA_Server_removeRepeatedJob(server, mon->sampleJobGuid);
}

/****************/
/* Subscription */
/****************/

UA_Subscription * UA_Subscription_new(UA_Session *session, UA_UInt32 subscriptionID) {
    UA_Subscription *new = UA_malloc(sizeof(UA_Subscription));
    if(!new)
        return NULL;
    new->session = session;
    new->subscriptionID = subscriptionID;
    new->sequenceNumber = 0;
    new->maxKeepAliveCount = 0;
    new->publishingEnabled = false;
    memset(&new->publishJobGuid, 0, sizeof(UA_Guid));
    new->publishJobIsRegistered = false;
    new->currentKeepAliveCount = 0;
    new->currentLifetimeCount = 0;
    new->lastMonitoredItemId = 0;
    new->state = UA_SUBSCRIPTIONSTATE_NORMAL; /* The first publish response is sent immediately */
    LIST_INIT(&new->retransmissionQueue);
    LIST_INIT(&new->MonitoredItems);
    return new;
}

void UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server) {
    Subscription_unregisterPublishJob(server, subscription);

    /* Delete monitored Items */
    UA_MonitoredItem *mon, *tmp_mon;
    LIST_FOREACH_SAFE(mon, &subscription->MonitoredItems, listEntry, tmp_mon) {
        LIST_REMOVE(mon, listEntry);
        MonitoredItem_delete(server, mon);
    }

    /* Delete Retransmission Queue */
    UA_NotificationMessageEntry *nme, *nme_tmp;
    LIST_FOREACH_SAFE(nme, &subscription->retransmissionQueue, listEntry, nme_tmp) {
        LIST_REMOVE(nme, listEntry);
        UA_NotificationMessage_deleteMembers(&nme->message);
        UA_free(nme);
    }
}

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub, UA_UInt32 monitoredItemID) {
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        if(mon->itemId == monitoredItemID)
            break;
    }
    return mon;
}

UA_StatusCode
UA_Subscription_deleteMonitoredItem(UA_Server *server, UA_Subscription *sub,
                                    UA_UInt32 monitoredItemID) {
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        if(mon->itemId == monitoredItemID) {
            LIST_REMOVE(mon, listEntry);
            MonitoredItem_delete(server, mon);
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

static size_t
countQueuedNotifications(UA_Subscription *sub, UA_Boolean *moreNotifications) {
    size_t notifications = 0;
    if(sub->publishingEnabled) {
        UA_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
            MonitoredItem_queuedValue *qv;
            TAILQ_FOREACH(qv, &mon->queue, listEntry) {
                if(notifications >= sub->notificationsPerPublish) {
                    *moreNotifications = true;
                    break;
                }
                notifications++;
            }
        }
    }
    return notifications;
}

static UA_StatusCode
prepareNotificationMessage(UA_Subscription *sub, UA_NotificationMessage *message,
                           size_t notifications) {
    /* Array of ExtensionObject to hold different kinds of notifications
       (currently only DataChangeNotifications) */
    message->notificationData = UA_Array_new(1, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(!message->notificationData)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    message->notificationDataSize = 1;

    /* Allocate Notification */
    UA_DataChangeNotification *dcn = UA_DataChangeNotification_new();
    if(!dcn)
        goto cleanup;
    UA_ExtensionObject *data = message->notificationData;
    data->encoding = UA_EXTENSIONOBJECT_DECODED;
    data->content.decoded.data = dcn;
    data->content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION];

    /* Allocate array of notifications */
    dcn->monitoredItems =
        UA_Array_new(notifications, &UA_TYPES[UA_TYPES_MONITOREDITEMNOTIFICATION]);
    if(!dcn->monitoredItems)
        goto cleanup;
    dcn->monitoredItemsSize = notifications;

    /* Move notifications into the response .. the point of no return */
    size_t l = 0;
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        MonitoredItem_queuedValue *qv, *qv_tmp;
        TAILQ_FOREACH_SAFE(qv, &mon->queue, listEntry, qv_tmp) {
            if(l >= notifications)
                return UA_STATUSCODE_GOOD;
            UA_MonitoredItemNotification *min = &dcn->monitoredItems[l];
            min->clientHandle = qv->clientHandle;
            min->value = qv->value;
            TAILQ_REMOVE(&mon->queue, qv, listEntry);
            UA_free(qv);
            mon->currentQueueSize--;
            l++;
        }
    }
    return UA_STATUSCODE_GOOD;
    
 cleanup:
    UA_NotificationMessage_deleteMembers(message);
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

void UA_Subscription_publishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session, "Subscription %u | "
                         "Publish Callback", sub->subscriptionID);

    /* Count the available notifications */
    UA_Boolean moreNotifications = false;
    size_t notifications = countQueuedNotifications(sub, &moreNotifications);

    /* Return if nothing to do */
    if(notifications == 0) {
        sub->currentKeepAliveCount++;
        if(sub->currentKeepAliveCount < sub->maxKeepAliveCount)
            return;
        UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                             "Sending a keepalive on subscription %u",
                             sub->subscriptionID)
    }

    /* Check if the securechannel is valid */
    UA_SecureChannel *channel = sub->session->channel;
    if(!channel)
        return;

    /* Dequeue a response */
    UA_PublishResponseEntry *pre = SIMPLEQ_FIRST(&sub->session->responseQueue);

    /* Cannot publish without a response */
    if(!pre) {
        UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                             "Cannot send a publish response on subscription %u, "
                             "since the publish queue is empty", sub->subscriptionID)
        if(sub->state != UA_SUBSCRIPTIONSTATE_LATE) {
            sub->state = UA_SUBSCRIPTIONSTATE_LATE;
        } else {
            sub->currentLifetimeCount++;
            if(sub->currentLifetimeCount > sub->lifeTimeCount) {
                UA_LOG_DEBUG_SESSION(server->config.logger, sub->session, "Subscription %u | "
                                     "End of lifetime for subscription", sub->subscriptionID);
                UA_Session_deleteSubscription(server, sub->session, sub->subscriptionID);
            }
        }
        return;
    }

    UA_PublishResponse *response = &pre->response;
    UA_NotificationMessage *message = &response->notificationMessage;
    UA_NotificationMessageEntry *retransmission = NULL;
    if(notifications > 0) {
        /* Allocate the retransmission entry */
        retransmission = malloc(sizeof(UA_NotificationMessageEntry));
        if(!retransmission) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                                   "Subscription %u | Could not allocate memory "
                                   "for retransmission", sub->subscriptionID);
            return;
        }
        /* Prepare the response */
        UA_StatusCode retval =
            prepareNotificationMessage(sub, message, notifications);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                                   "Subscription %u | Could not prepare the "
                                   "notification message", sub->subscriptionID);
            UA_free(retransmission);
            return;
        }
    }

    /* <-- The point of no return --> */

    /* Remove the response from the response queue */
    SIMPLEQ_REMOVE_HEAD(&sub->session->responseQueue, listEntry);

    /* Set up the response */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->subscriptionId = sub->subscriptionID;
    response->moreNotifications = moreNotifications;
    message->publishTime = response->responseHeader.timestamp;
    if(notifications == 0) {
        /* Send sequence number for the next notification */
        message->sequenceNumber = sub->sequenceNumber + 1;
    } else {
        /* Increase the sequence number */
        message->sequenceNumber = ++sub->sequenceNumber;

        /* Put the notification message into the retransmission queue. This needs to
         * be done here, so that the message itself is included in the available
         * sequence numbers for acknowledgement. */
        retransmission->message = response->notificationMessage;
        LIST_INSERT_HEAD(&sub->retransmissionQueue, retransmission, listEntry);
    }

    /* Get the available sequence numbers from the retransmission queue */
    size_t available = 0;
    UA_NotificationMessageEntry *nme;
    LIST_FOREACH(nme, &sub->retransmissionQueue, listEntry)
        available++;
    // cppcheck-suppress knownConditionTrueFalse
    if(available > 0) {
        response->availableSequenceNumbers = UA_alloca(available * sizeof(UA_UInt32));
        response->availableSequenceNumbersSize = available;
    }
    size_t i = 0;
    LIST_FOREACH(nme, &sub->retransmissionQueue, listEntry) {
        response->availableSequenceNumbers[i] = nme->message.sequenceNumber;
        i++;
    }

    /* Send the response */
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | Sending out a publish response with %u "
                         "notifications", sub->subscriptionID, (UA_UInt32)notifications);
    UA_SecureChannel_sendBinaryMessage(sub->session->channel, pre->requestId, response,
                                       &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Reset subscription state to normal. */
    sub->state = UA_SUBSCRIPTIONSTATE_NORMAL;
    sub->currentKeepAliveCount = 0;
    sub->currentLifetimeCount = 0;
    
    /* Free the response */
    UA_Array_delete(response->results, response->resultsSize,
                    &UA_TYPES[UA_TYPES_UINT32]);
    UA_free(pre); /* no need for UA_PublishResponse_deleteMembers */

    /* Repeat if there are more notifications to send */
    if(moreNotifications)
        UA_Subscription_publishCallback(server, sub);
}

UA_StatusCode
Subscription_registerPublishJob(UA_Server *server, UA_Subscription *sub) {
    if(sub->publishJobIsRegistered)
        return UA_STATUSCODE_GOOD;

    UA_Job job;
    job.type = UA_JOBTYPE_METHODCALL;
    job.job.methodCall.method = (UA_ServerCallback)UA_Subscription_publishCallback;
    job.job.methodCall.data = sub;
    UA_StatusCode retval =
        UA_Server_addRepeatedJob(server, job, (UA_UInt32)sub->publishingInterval,
                                 &sub->publishJobGuid);
    if(retval == UA_STATUSCODE_GOOD)
        sub->publishJobIsRegistered = true;
    return retval;
}

UA_StatusCode
Subscription_unregisterPublishJob(UA_Server *server, UA_Subscription *sub) {
    if(!sub->publishJobIsRegistered)
        return UA_STATUSCODE_GOOD;
    sub->publishJobIsRegistered = false;
    return UA_Server_removeRepeatedJob(server, sub->publishJobGuid);
}

/* When the session has publish requests stored but the last subscription is
   deleted... Send out empty responses */
void
UA_Subscription_answerPublishRequestsNoSubscription(UA_Server *server, UA_NodeId *sessionToken) {
    /* Get session */
    UA_Session *session = UA_SessionManager_getSession(&server->sessionManager, sessionToken);
    UA_NodeId_delete(sessionToken);

    /* No session or there are remaining subscriptions */
    if(!session || LIST_FIRST(&session->serverSubscriptions))
        return;

    /* Send a response for every queued request */
    UA_PublishResponseEntry *pre;
    while((pre = SIMPLEQ_FIRST(&session->responseQueue))) {
        SIMPLEQ_REMOVE_HEAD(&session->responseQueue, listEntry);
        UA_PublishResponse *response = &pre->response;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOSUBSCRIPTION;
        response->responseHeader.timestamp = UA_DateTime_now();
        UA_SecureChannel_sendBinaryMessage(session->channel, pre->requestId, response,
                                           &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_PublishResponse_deleteMembers(response);
        UA_free(pre);
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
