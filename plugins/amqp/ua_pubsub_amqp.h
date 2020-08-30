/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * @author Waheed Ejaz (IMD, University of Rostock)
 *
 */

#ifndef OPEN62541_UA_PUBSUB_AMQP_H
#define OPEN62541_UA_PUBSUB_AMQP_H

#include <open62541/plugin/pubsub.h>
#include <open62541/network_tcp.h>

#include <proton/connection_driver.h>
#include <proton/session.h>
#include <proton/link.h>
#include <proton/message.h>


#define MAX_AMQP_LINKS 2
#define SENDER_LINK 0
#define RECEIVER_LINK 1

/* AMQP network layer specific internal data */
typedef struct {
    UA_NetworkAddressUrlDataType address;
    UA_Connection              *ua_connection;
    pn_connection_driver_t     *driver;
    pn_session_t               *session;

    UA_Boolean      openLink;
    /* Total 2 links, 1 link can either sender or receiver */
    pn_link_t       * links [ MAX_AMQP_LINKS ];
    UA_Boolean      sender_link_ready;
    pn_message_t    * message;
    pn_rwbytes_t    send_buffer;
    pn_rwbytes_t    message_buffer;

    UA_UInt32       sequence_no;
    UA_UInt32       acknowledged_no;

} UA_AmqpContext;

UA_PubSubTransportLayer
UA_PubSubTransportLayerAMQP(void);



#endif //OPEN62541_UA_PUBSUB_AMQP_H
