#ifndef OPCUA_APPLICATION_H_
#define OPCUA_APPLICATION_H_

#include "opcua.h"
#include "ua_namespace.h"
#include "ua_indexedList.h"

typedef struct Application_T {
	UA_ApplicationDescription *description;
	UA_indexedList_List *namespaces; // each entry is a namespace
} Application;

extern Application appMockup;
void appMockup_init();
#endif
