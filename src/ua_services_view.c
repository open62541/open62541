#include "ua_services.h"

UA_Int32 Service_Read(SL_Channel *channel, const UA_ReadRequest *request, UA_ReadResponse *response) {
	UA_NodeId_printf("BrowseService - view=",&(p->view.viewId));
	UA_Int32 i = 0;
	for (i=0;p->nodesToBrowseSize > 0 && i<p->nodesToBrowseSize;i++) {
		UA_NodeId_printf("BrowseService - nodesToBrowse=", &(p->nodesToBrowse[i]->nodeId));
	}
}
