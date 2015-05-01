#ifdef ENABLESUBSCRIPTIONS
#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"

// Remove later, debugging only
#include "ua_log.h"

UA_Int32 Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                     const UA_CreateSubscriptionRequest *request,
                                     UA_CreateSubscriptionResponse *response) {
    UA_Subscription *newSubscription;
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    
    response->subscriptionId = ++(server->subscriptionManager.LastSessionID);
    if      (request->requestedPublishingInterval > server->subscriptionManager.GlobalPublishingInterval.maxValue) response->revisedPublishingInterval = server->subscriptionManager.GlobalPublishingInterval.maxValue ;
    else if (request->requestedPublishingInterval < server->subscriptionManager.GlobalPublishingInterval.minValue) response->revisedPublishingInterval = server->subscriptionManager.GlobalPublishingInterval.minValue ;
    else response->revisedPublishingInterval = request->requestedPublishingInterval ;
    
    if      (request->requestedLifetimeCount > server->subscriptionManager.GlobalLifeTimeCount.maxValue) response->revisedLifetimeCount = server->subscriptionManager.GlobalLifeTimeCount.maxValue ;
    else if (request->requestedLifetimeCount < server->subscriptionManager.GlobalLifeTimeCount.minValue) response->revisedLifetimeCount = server->subscriptionManager.GlobalLifeTimeCount.minValue ;
    else response->revisedLifetimeCount = request->requestedLifetimeCount ;
    
    if      (request->requestedMaxKeepAliveCount > server->subscriptionManager.GlobalKeepAliveCount.maxValue) response->revisedMaxKeepAliveCount = server->subscriptionManager.GlobalKeepAliveCount.maxValue ;
    else if (request->requestedMaxKeepAliveCount < server->subscriptionManager.GlobalKeepAliveCount.minValue) response->revisedMaxKeepAliveCount = server->subscriptionManager.GlobalKeepAliveCount.minValue ;
    else response->revisedMaxKeepAliveCount = request->requestedMaxKeepAliveCount ;
    
    //maxNotificationsPerPublish ?
    //Type??
    newSubscription = UA_Subscription_new(response->subscriptionId);
    SubscriptionManager_addSubscription(&(server->subscriptionManager), newSubscription);    
    
    return (UA_Int32) 0;
}

#endif //#ifdef ENABLESUBSCRIPTIONS
