/*
 * ua_stack_session.h
 *
 *  Created on: 05.06.2014
 *      Author: root
 */

#ifndef UA_STACK_SESSION_H_
#define UA_STACK_SESSION_H_


#include "ua_stack_channel.h"

struct UA_Session;
typedef struct UA_Session UA_Session;
typedef UA_Int32(*UA_Session_idProvider)(UA_NodeId *newSessionId);
/**
 * @brief creates a session object
 * @param newSession
 * @return error code
 */
UA_Int32 UA_Session_new(UA_Session **newSession);
/**
 * @brief inits a session object
 * @param session
 * @param sessionName
 * @param requestedSessionTimeout
 * @param maxRequestMessageSize
 * @param maxResponseMessageSize
 * @param idProvider
 * @param timeout
 * @return error code
 */
UA_Int32 UA_Session_init(UA_Session *session, UA_String *sessionName, UA_Double requestedSessionTimeout,
		UA_UInt32 maxRequestMessageSize,
		UA_UInt32 maxResponseMessageSize,
		UA_Session_idProvider idProvider,
		UA_Int64 timeout);
UA_Int32 UA_Session_delete(UA_Session *session);
/**
 * @brief compares two session objects
 * @param session1
 * @param session2
 * @return UA_TRUE if it is the same session, UA_FALSE else
 */
UA_Boolean UA_Session_compare(UA_Session *session1, UA_Session *session2);
/**
 * @brief compares two sessions by their authentication token
 * @param session
 * @param token
 * @return UA_EQUAL if the session token matches the session UA_NOT_EQUAL
 */
UA_Boolean UA_Session_compareByToken(UA_Session *session, UA_NodeId *token);
/**
 * @brief compares two sessions by their session id
 * @param session
 * @param sessionId
 * @return UA_EQUAL if the session identifier matches the session UA_NOT_EQUAL
 */
UA_Boolean UA_Session_compareById(UA_Session *session, UA_NodeId *sessionId);
/**
 * @brief binds a channel to a session
 * @param session
 * @param channel
 * @return error code
 */
UA_Int32 UA_Session_bind(UA_Session *session, SL_Channel *channel);
/**
 * @brief checks if the given channel is related to the session
 * @param session
 * @param channel
 * @return UA_TRUE if there is a relation between session and given channel
 */
UA_Boolean UA_Session_verifyChannel(UA_Session *session, SL_Channel *channel);
/**
 * @brief If any activity on a session happens, the timeout must be extended
 * @param session
 * @return error code
 */
UA_Int32 UA_Session_updateLifetime(UA_Session *session);
/**
 * @brief Gets the session identifier (UA_NodeId)
 * @param session session from which the identifier should be returned
 * @param sessionId return value
 * @return error code
 */
UA_Int32 UA_Session_getId(UA_Session *session, UA_NodeId *sessionId);
/**
 * @brief Gets the session authentication token
 * @param session session from which the token should be returned
 * @param authenticationToken return value
 * @return error code
 */
UA_Int32 UA_Session_getToken(UA_Session *session, UA_NodeId *authenticationToken);
/**
 * @brief Gets the channel on which the session is currently running
 * @param session session from which the channel should be returned
 * @param channel return value
 * @return
 */
UA_Int32 UA_Session_getChannel(UA_Session *session, SL_Channel **channel);
/**
 * @brief Gets the sessions pending lifetime (calculated from the timeout which was set)
 * @param session session from which the lifetime should be returned
 * @param pendingLifetime return value
 * @return error code
 */
UA_Int32 UA_Session_getPendingLifetime(UA_Session *session,UA_Double *pendingLifetime);
/**
 * @brief Gets the pointer to the application
 * @param session session from which the application pointer should be returned
 * @param application return value
 * @return  error code
 */
UA_Int32 UA_Session_getApplicationPointer(UA_Session *session, Application** application);
/**
 * @brief Sets the application pointer to the application
 * @param session session of which the application pointer should be set
 * @param application return value
 * @return error code
 */
UA_Int32 UA_Session_setApplicationPointer(UA_Session *session, Application* application);


#endif /* UA_STACK_SESSION_H_ */
