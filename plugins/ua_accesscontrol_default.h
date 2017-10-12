/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_ACCESSCONTROL_DEFAULT_H_
#define UA_ACCESSCONTROL_DEFAULT_H_

#include "ua_server.h"

#ifdef __cplusplus
extern "C" {
#endif

UA_StatusCode UA_EXPORT
activateSession_default(const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext);

void UA_EXPORT
closeSession_default(const UA_NodeId *sessionId, void *sessionContext);

UA_UInt32 UA_EXPORT
getUserRightsMask_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext);

UA_Byte UA_EXPORT
getUserAccessLevel_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext);

UA_Boolean UA_EXPORT
getUserExecutable_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext);

UA_Boolean UA_EXPORT
getUserExecutableOnObject_default(const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext);

UA_Boolean UA_EXPORT
allowAddNode_default(const UA_NodeId *sessionId, void *sessionContext,
                     const UA_AddNodesItem *item);

UA_Boolean UA_EXPORT
allowAddReference_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_AddReferencesItem *item);

UA_Boolean UA_EXPORT
allowDeleteNode_default(const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item);

UA_Boolean UA_EXPORT
allowDeleteReference_default(const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item);

#ifdef __cplusplus
}
#endif

#endif /* UA_ACCESSCONTROL_DEFAULT_H_ */
