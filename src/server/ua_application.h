#ifndef OPCUA_APPLICATION_H_
#define OPCUA_APPLICATION_H_

#include "ua_types.h"
#include "ua_namespace.h"
#include "util/ua_indexedList.h"

typedef struct Application {
	UA_ApplicationDescription *description;
	UA_indexedList_List *namespaces; // each entry is a namespace
} Application;

extern Application appMockup;
void appMockup_init();
#endif
