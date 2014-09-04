#ifndef UA_SESSION_H_
#define UA_SESSION_H_

#include "ua_types.h"
#include "ua_securechannel.h"

/**
 *  @ingroup internal
 *
 *  @defgroup session Session
 */

struct UA_Session {
	UA_ApplicationDescription clientDescription;
	UA_String sessionName;
	UA_NodeId authenticationToken;
	UA_NodeId sessionId;
	UA_UInt32 maxRequestMessageSize;
	UA_UInt32 maxResponseMessageSize;
	UA_Int64 timeout;
	UA_DateTime validTill;
	UA_SecureChannel *channel;
};

UA_Int32 UA_Session_new(UA_Session **session);
UA_Int32 UA_Session_init(UA_Session *session);
UA_Int32 UA_Session_delete(UA_Session *session);
UA_Int32 UA_Session_deleteMembers(UA_Session *session);

/** Compares two session objects */
UA_Boolean UA_Session_compare(UA_Session *session1, UA_Session *session2);

/** If any activity on a session happens, the timeout must be extended */
UA_Int32 UA_Session_updateLifetime(UA_Session *session);

/** Gets the sessions pending lifetime (calculated from the timeout which was set) */
UA_Int32 UA_Session_getPendingLifetime(UA_Session *session,UA_Double *pendingLifetime);

#endif /* UA_SESSION_H_ */
