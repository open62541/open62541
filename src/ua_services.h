#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include "opcua.h"
#include "ua_application.h"
#include "ua_statuscodes.h"
#include "ua_transportLayer.h"

/* Part 4: 5.4 Discovery Service Set */
// service_findservers
// service_getendpoints
// service_registerserver

/* Part 4: 5.5 SecureChannel Service Set */
// service_opensecurechannel
// service_closesecurechannel

/* Part 4: 5.6 Session Service Set */
UA_Int32 service_createsession(UA_SL_Channel *channel, UA_CreateSessionRequest *request, UA_CreateSessionResponse *response);
UA_Int32 service_activatesession(UA_SL_Channel *channel, UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response);
UA_Int32 service_closesession(UA_SL_Channel *channel, UA_CloseSessionRequest *request, UA_CloseSessionResponse *response);
// service_cancel

/* Part 4: 5.7 NodeManagement Service Set */
// service_addnodes
// service_addreferences
// service_deletenodes
// service_deletereferences

/* Part 4: 5.8 View Service Set */
// service_browse
// service_browsenext
// service_translatebrowsepathstonodeids
// service_registernodes
// service_unregisternodes

/* Part 4: 5.9 Query Service Set */
// service_queryfirst
// service_querynext

/* Part 4: 5.10 Attribute Service Set */
UA_Int32 service_read(UA_Application *app, UA_ReadRequest *request, UA_ReadResponse *response);
// service_historyread;
// service_write;
// service_historyupdate;

/* Part 4: 5.11 Method Service Set */
// service_call

/* Part 4: 5.12 MonitoredItem Service Set */
// service_createmonitoreditems
// service_modifymonitoreditems
// service_setmonitoringmode
// service_settriggering
// service_deletemonitoreditems

/* Part 4: 5.13 Subscription Service Set */
// service_createsubscription
// service_modifysubscription
// service_setpublishingmode
// service_publish
// service_republish
// service_transfersubscription
// service_deletesubscription

#endif
