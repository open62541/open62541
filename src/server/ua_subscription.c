#include "ua_subscription.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_nodestore.h"

/*****************/
/* MonitoredItem */
/*****************/

UA_MonitoredItem * UA_MonitoredItem_new() {
    UA_MonitoredItem *new = UA_malloc(sizeof(UA_MonitoredItem));
    new->subscription = NULL;
    new->currentQueueSize = 0;
    new->maxQueueSize = 0;
    new->monitoredItemType = MONITOREDITEM_TYPE_CHANGENOTIFY; // TODO: This is currently hardcoded;
    TAILQ_INIT(&new->queue);
    UA_NodeId_init(&new->monitoredNodeId);
    new->lastSampledValue = UA_BYTESTRING_NULL;
    memset(&new->sampleJobGuid, 0, sizeof(UA_Guid));
    new->sampleJobIsRegistered = false;
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
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    UA_NodeId_deleteMembers(&monitoredItem->monitoredNodeId);
    UA_free(monitoredItem);
}

static void SampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    if(monitoredItem->monitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY) {
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Cannot process a monitoreditem that is not a data change notification");
        return;
    }

    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Sampling the value on monitoreditem %u", monitoredItem->itemId);

    MonitoredItem_queuedValue *newvalue = UA_malloc(sizeof(MonitoredItem_queuedValue));
    if(!newvalue)
        return;
    UA_DataValue_init(&newvalue->value);
    newvalue->clientHandle = monitoredItem->clientHandle;
    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Creating a sample with client handle %u", newvalue->clientHandle);
  
    /* Read the value */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = monitoredItem->monitoredNodeId;
    rvid.attributeId = monitoredItem->attributeID;
    UA_Subscription *sub = monitoredItem->subscription;
    Service_Read_single(server, sub->session, monitoredItem->timestampsToReturn, &rvid, &newvalue->value);

    /* encode to see if the data has changed */
    size_t binsize = UA_calcSizeBinary(&newvalue->value.value, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_ByteString newValueAsByteString;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&newValueAsByteString, binsize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }
    size_t encodingOffset = 0;
    retval = UA_encodeBinary(&newvalue->value.value, &UA_TYPES[UA_TYPES_VARIANT],
                             &newValueAsByteString, &encodingOffset);

    /* error or the content has not changed */
    if(retval != UA_STATUSCODE_GOOD ||
       (monitoredItem->lastSampledValue.data &&
        UA_String_equal(&newValueAsByteString, &monitoredItem->lastSampledValue))) {
        UA_ByteString_deleteMembers(&newValueAsByteString);
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Do not sample since the data value has not changed");
        return;
    }
  
    /* do we have space? */
    if(monitoredItem->currentQueueSize >= monitoredItem->maxQueueSize) {
        if(!monitoredItem->discardOldest) {
            // We cannot remove the oldest value and theres no queue space left. We're done here.
            UA_ByteString_deleteMembers(&newValueAsByteString);
            UA_DataValue_deleteMembers(&newvalue->value);
            UA_free(newvalue);
            return;
        }
        MonitoredItem_queuedValue *queueItem = TAILQ_LAST(&monitoredItem->queue, QueueOfQueueDataValues);
        TAILQ_REMOVE(&monitoredItem->queue, queueItem, listEntry);
        UA_DataValue_deleteMembers(&queueItem->value);
        UA_free(queueItem);
        monitoredItem->currentQueueSize--;
    }
  
    /* add the sample */
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    monitoredItem->lastSampledValue = newValueAsByteString;
    TAILQ_INSERT_TAIL(&monitoredItem->queue, newvalue, listEntry);
    monitoredItem->currentQueueSize++;
}

UA_StatusCode MonitoredItem_registerSampleJob(UA_Server *server, UA_MonitoredItem *mon) {
    //SampleCallback(server, mon);
    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
                  .job.methodCall = {.method = (UA_ServerCallback)SampleCallback, .data = mon} };
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, job, (UA_UInt32)mon->samplingInterval,
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
	new->state = UA_SUBSCRIPTIONSTATE_LATE; /* The first publish response is sent immediately */
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

void UA_Subscription_publishCallback(UA_Server *server, UA_Subscription *sub) {
    /* Count the available notifications */
    size_t notifications = 0;
    UA_Boolean moreNotifications = false;
    if(sub->publishingEnabled) {
        UA_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
            MonitoredItem_queuedValue *qv;
            TAILQ_FOREACH(qv, &mon->queue, listEntry) {
                if(notifications >= sub->notificationsPerPublish) {
                    moreNotifications = true;
                    break;
                }
                notifications++;
            }
        }
    }

    /* Return if nothing to do */
    if(notifications == 0) {
        sub->currentKeepAliveCount++;
        if(sub->currentKeepAliveCount < sub->maxKeepAliveCount)
            return;
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
            "Sending out a keepalive on subscription %u on securechannel %u", sub->subscriptionID,
            sub->session->authenticationToken.identifier.numeric);
    } else {
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
            "Sending out a publish response on subscription %u on securechannel %u " \
            "with %u notifications", sub->subscriptionID,
            sub->session->authenticationToken.identifier.numeric, (UA_UInt32)notifications);
    }

    /* Check if the securechannel is valid */
    UA_SecureChannel *channel = sub->session->channel;
    if(!channel)
        return;

	/* Dequeue a response */
	UA_PublishResponseEntry *pre = SIMPLEQ_FIRST(&sub->session->responseQueue);
	if(!pre) {
		UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
			"Cannot send a publish response on subscription %u " \
			"since the publish queue is empty on session %u",
			sub->subscriptionID, sub->session->authenticationToken.identifier.numeric);
		if(sub->state != UA_SUBSCRIPTIONSTATE_LATE) {
			sub->state = UA_SUBSCRIPTIONSTATE_LATE;
		} else {
			sub->currentLifetimeCount++;
			if(sub->currentLifetimeCount > sub->lifeTimeCount) {
				UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
					"End of lifetime for subscription %u on session %u",
					sub->subscriptionID, sub->session->authenticationToken.identifier.numeric);
				UA_Session_deleteSubscription(server, sub->session, sub->subscriptionID);
			}
		}
		return;
	}
	SIMPLEQ_REMOVE_HEAD(&sub->session->responseQueue, listEntry);

    UA_PublishResponse *response = &pre->response;
    UA_UInt32 requestId = pre->requestId;

	/* We have a request. Reset state to normal. */
	sub->state = UA_SUBSCRIPTIONSTATE_NORMAL;
    sub->currentKeepAliveCount = 0;
    sub->currentLifetimeCount = 0;

	/* Prepare the response */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->subscriptionId = sub->subscriptionID;
	response->moreNotifications = moreNotifications;
    UA_NotificationMessage *message = &response->notificationMessage;
    message->publishTime = response->responseHeader.timestamp;
    if(notifications > 0) {
        message->sequenceNumber = ++sub->sequenceNumber;

        /* Collect the notification messages */
        message->notificationData = UA_ExtensionObject_new();
        message->notificationDataSize = 1;
        UA_ExtensionObject *data = message->notificationData;
        UA_DataChangeNotification *dcn = UA_DataChangeNotification_new();
        dcn->monitoredItems = UA_Array_new(notifications, &UA_TYPES[UA_TYPES_MONITOREDITEMNOTIFICATION]);
        dcn->monitoredItemsSize = notifications;
        size_t l = 0;
        UA_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
            MonitoredItem_queuedValue *qv, *qv_tmp;
            TAILQ_FOREACH_SAFE(qv, &mon->queue, listEntry, qv_tmp) {
                if(notifications <= l)
                    break;
                UA_MonitoredItemNotification *min = &dcn->monitoredItems[l];
                min->clientHandle = qv->clientHandle;
                min->value = qv->value;
                TAILQ_REMOVE(&mon->queue, qv, listEntry);
                UA_free(qv);
                mon->currentQueueSize--;
                l++;
            }
        }
        data->encoding = UA_EXTENSIONOBJECT_DECODED;
        data->content.decoded.data = dcn;
        data->content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION];

        /* Put the notification message into the retransmission queue */
        UA_NotificationMessageEntry *retransmission = malloc(sizeof(UA_NotificationMessageEntry));
        retransmission->message = response->notificationMessage;
        LIST_INSERT_HEAD(&sub->retransmissionQueue, retransmission, listEntry);
    } else /* Send sequence number for the next notification */
		message->sequenceNumber = sub->sequenceNumber + 1;

    /* Get the available sequence numbers from the retransmission queue */
    size_t available = 0;
    UA_NotificationMessageEntry *nme;
    LIST_FOREACH(nme, &sub->retransmissionQueue, listEntry)
        available++;
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
    UA_SecureChannel_sendBinaryMessage(sub->session->channel, requestId, response,
                                       &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Remove the queued request */
    UA_NotificationMessage_init(&response->notificationMessage); /* The notification message was put into the queue and is not freed */
    response->availableSequenceNumbers = NULL; /* stack-allocated */
    response->availableSequenceNumbersSize = 0;
    UA_PublishResponse_deleteMembers(&pre->response);
    UA_free(pre);

    /* Repeat if there are more notifications to send */
    if(moreNotifications)
        UA_Subscription_publishCallback(server, sub);
}

UA_StatusCode Subscription_registerPublishJob(UA_Server *server, UA_Subscription *sub) {
    UA_Job job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL,
                           .job.methodCall = {.method = (UA_ServerCallback)UA_Subscription_publishCallback, .data = sub} };
    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Adding a subscription with %i millisec interval", (int)sub->publishingInterval);
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, job,
                                                    (UA_UInt32)sub->publishingInterval,
                                                    &sub->publishJobGuid);
    if(retval == UA_STATUSCODE_GOOD)
        sub->publishJobIsRegistered = true;
    else
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not register a subscription publication job with status code 0x%08x\n",
                     retval);
    return retval;
}

UA_StatusCode Subscription_unregisterPublishJob(UA_Server *server, UA_Subscription *sub) {
    if(!sub->publishJobIsRegistered)
        return UA_STATUSCODE_GOOD;
    sub->publishJobIsRegistered = false;
    UA_StatusCode retval = UA_Server_removeRepeatedJob(server, sub->publishJobGuid);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not remove a subscription publication job with status code 0x%08x\n",
                     retval);
    return retval;
}
