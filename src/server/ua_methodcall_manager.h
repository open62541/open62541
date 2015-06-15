#ifndef UA_METHODCALL_H
#define UA_METHODCALL_H

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodes.h"

typedef struct UA_MethodCall_Manager_s {
    UA_Int32 placeHolder;
} UA_MethodCall_Manager;

UA_MethodCall_Manager *UA_MethodCallManager_new(void);
#endif