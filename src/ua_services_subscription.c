/*
 * ua_services_subscription.c
 *
 *  Created on: 18.06.2014
 *      Author: root
 */
#include "ua_services.h"

UA_Int32 Service_CreateSubscription(SL_Channel *channel, const UA_CreateSubscriptionRequest *request,
                                   UA_CreateSubscriptionResponse *response)
{

	response->subscriptionId = 42;
	response->revisedPublishingInterval = 10000;
	response->revisedLifetimeCount = 120000;
	response->revisedMaxKeepAliveCount = 50;
	return UA_SUCCESS;
}
