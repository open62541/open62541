/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * @author Waheed Ejaz (IMD, University of Rostock)
 *
 */

#ifndef OPEN62541_UA_AMQP_ADAPTOR_H
#define OPEN62541_UA_AMQP_ADAPTOR_H

#include "ua_pubsub_amqp.h"


/**
 * @brief UA_AmqpDisconnect
 * @param ctx pointer to the AMQP context
 * @return None
 */
void UA_AmqpDisconnect(UA_AmqpContext *ctx);

/**
 * @brief Connect to AMQP 1.0 broker
 * @param amqpCtx pointer to the AMQP data
 * @return UA_StatusCode
 */
UA_StatusCode UA_AmqpConnect(UA_AmqpContext *amqpCtx, UA_NetworkAddressUrlDataType address);

/**
 * @brief Sends a data message to queue specified by sender link
 * @param amqpCtx Pointer to the amqpCtx
 * @param sender PN_link associated with queue
 * @param buf message data to be sent
 * @return UA_STATUSCODE
 */
UA_StatusCode publishAmqp(UA_AmqpContext *amqpCtx, UA_String queue,
                          const UA_ByteString *buf);

/**
 * @brief Sends out all tx buffer on the socket
 *        Receive any incoming traffic on the socket
 *        Handle any pending events
 * @param amqpCtx pointer AMQP context
 * @param timeout timeout for listening on the socket
 */
UA_StatusCode yieldAmqp(UA_AmqpContext *amqpCtx, UA_UInt16 timeout);

#endif //OPEN62541_UA_AMQP_ADAPTOR_H
