//
// Created by aribr on 30/01/2018.
//

#ifndef UA_SUBSCRIPTION_EVENTS_H_
#define UA_SUBSCRIPTION_EVENTS_H_

#ifdef UA_ENABLE_EVENTS

#include "ua_subscription.h"

#define UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE 4035

/* generates a unique eventId */
UA_StatusCode UA_Event_generateEventId(UA_Server *server, UA_ByteString *outId);

/* returns the eventId of a node representation of an event */
UA_StatusCode UA_Server_getEventId(UA_Server *server, UA_NodeId *eventNodeId, UA_ByteString *outId);

/* filters the given event with the given filter and writes the results into a notification */
UA_StatusCode UA_Server_filterEvent(UA_Server *server, UA_NodeId *eventNode, UA_EventFilter *filter,
                                    UA_EventNotification *notification);

/* filters an event according to the filter specified by mon and then adds it to mons notification queue */
UA_StatusCode UA_Event_addEventToMonitoredItem(UA_Server *server, UA_NodeId *event, UA_MonitoredItem *mon);

/* Searches for an attribute of an attribute of an event with the name 'name' and the depth form the event
 * relativePathSize.
 * Returns the browsePathResult of searching for that node */
void UA_Event_findVariableNode(UA_Server *server, UA_QualifiedName *name, size_t relativePathSize, UA_NodeId *event,
                               UA_BrowsePathResult *out);

#ifdef UA_DEBUG_EVENTS
void UA_Event_generateExampleEvent(UA_Server *server);
#endif

#endif

#endif /* UA_SUBSCRIPTION_EVENTS_H_ */
