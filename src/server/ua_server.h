#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#include "ua_application.h"

struct UA_Server;
typedef struct UA_Server {
	// SL_ChannelManager *cm;
	// UA_SessionManager sm;
	UA_UInt32 applicationsSize;
	UA_Application *applications;
	/** Every thread needs to init its logger variable. The server-object stores a
		function pointer and logger configuration for that purpose. */
	void (*logger_init)(void *config);
	/** Configuration for the logger */
	void *logger_configuration;
} UA_Server;

#endif /* UA_SERVER_H_ */
