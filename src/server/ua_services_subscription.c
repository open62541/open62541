/*
 * ua_services_subscription.c
 *
 *  Created on: 18.06.2014
 *      Author: root
 */
#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_CreateSubscription(UA_Server *server, UA_Session *session,
									const UA_CreateSubscriptionRequest *request,
									UA_CreateSubscriptionResponse *response) {

	response->subscriptionId = 42;
	response->revisedPublishingInterval = 100000;
	response->revisedLifetimeCount = 120000;
	response->revisedMaxKeepAliveCount = 50;
	return UA_SUCCESS;
}

UA_Int32 Service_Publish(UA_Server *server, UA_Session *session,
						 const UA_PublishRequest *request,
						 UA_PublishResponse *response) {

	response->subscriptionId = 42;
	response->notificationMessage.sequenceNumber = 1;
	response->notificationMessage.publishTime = UA_DateTime_now();
	return UA_SUCCESS;
}

UA_Int32 Service_SetPublishingMode(UA_Server *server, UA_Session *session,
								   const UA_SetPublishingModeRequest *request,
                                   UA_SetPublishingModeResponse *response) {
	response->diagnosticInfos = UA_NULL;
	response->results = UA_NULL;
	response->resultsSize = 0;
	return UA_SUCCESS;
}
