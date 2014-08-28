#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include "ua_application.h"

struct UA_Server;
typedef struct UA_Server {
	// SL_ChannelManager *cm;
	// UA_SessionManager sm;
	UA_Application *application;
} UA_Server;

#endif /* UA_SERVER_H_ */
