/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_ACCESSCONTROL_DEFAULT_H_
#define UA_ACCESSCONTROL_DEFAULT_H_

#include "ua_server.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const UA_Boolean enableAnonymousLogin;
extern const UA_Boolean enableUsernamePasswordLogin;
extern const size_t usernamePasswordsSize;
extern const UA_UsernamePasswordLogin *usernamePasswords;

UA_EXPORT UA_StatusCode
activateSession_default(const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken, void **sessionHandle);

UA_EXPORT void
closeSession_default(const UA_NodeId *sessionId, void *sessionHandle);

UA_EXPORT UA_UInt32
getUserRightsMask_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId);

UA_EXPORT UA_Byte
getUserAccessLevel_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId);

UA_EXPORT UA_Boolean
getUserExecutable_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId);

UA_EXPORT UA_Boolean
getUserExecutableOnObject_default(const UA_NodeId *sessionId, void *sessionHandle,
                                  const UA_NodeId *methodId, const UA_NodeId *objectId);

UA_EXPORT UA_Boolean
allowAddNode_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_AddNodesItem *item);

UA_EXPORT UA_Boolean
allowAddReference_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_AddReferencesItem *item);

UA_EXPORT UA_Boolean
allowDeleteNode_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_DeleteNodesItem *item);
      
UA_EXPORT UA_Boolean
allowDeleteReference_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_DeleteReferencesItem *item);

#ifdef __cplusplus
}
#endif

#endif /* UA_ACCESSCONTROL_DEFAULT_H_ */
