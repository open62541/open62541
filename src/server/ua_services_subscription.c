#ifdef ENABLESUBSCRIPTIONS
#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"


#include "ua_log.h" // Remove later, debugging only

#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
    if (SRC > BOUNDS.maxValue) DST = BOUNDS.maxValue; \
    else if (SRC < BOUNDS.minValue) DST = BOUNDS.minValue; \
    else DST = SRC; \
    }
    
UA_Int32 Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                     const UA_CreateSubscriptionRequest *request,
                                     UA_CreateSubscriptionResponse *response) {
    UA_Subscription *newSubscription;
    
    // Verify Session
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    // Create Subscription and Response
    response->subscriptionId = ++(server->subscriptionManager.LastSessionID);
    newSubscription = UA_Subscription_new(response->subscriptionId);
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalPublishingInterval, request->requestedPublishingInterval, response->revisedPublishingInterval);
    newSubscription->PublishingInterval = response->revisedPublishingInterval;
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalLifeTimeCount,  request->requestedLifetimeCount, response->revisedLifetimeCount);
    newSubscription->LifeTime = (UA_UInt32_BoundedValue)  { .minValue=server->subscriptionManager.GlobalLifeTimeCount.minValue, .maxValue=server->subscriptionManager.GlobalLifeTimeCount.maxValue, .currentValue=response->revisedLifetimeCount};
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalKeepAliveCount, request->requestedMaxKeepAliveCount, response->revisedMaxKeepAliveCount);
    newSubscription->KeepAliveCount = (UA_UInt32_BoundedValue)  { .minValue=server->subscriptionManager.GlobalKeepAliveCount.minValue, .maxValue=server->subscriptionManager.GlobalKeepAliveCount.maxValue, .currentValue=response->revisedMaxKeepAliveCount};
    
    newSubscription->NotificationsPerPublish = request->maxNotificationsPerPublish;
    newSubscription->PublishingMode          = request->publishingEnabled;
    newSubscription->Priority                = request->priority;
    
    SubscriptionManager_addSubscription(&(server->subscriptionManager), newSubscription);    
    
    return (UA_Int32) 0;
}

#endif //#ifdef ENABLESUBSCRIPTIONS
