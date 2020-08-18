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

#include <proton/connection.h>
#include <proton/delivery.h>
#include <proton/connection_driver.h>
#include <proton/event.h>
#include <proton/terminus.h>
#include <proton/link.h>
#include <proton/message.h>
#include <proton/session.h>

#define MAX_AMQP_LINKS 2
#define SENDER_LINK 0
#define RECEIVER_LINK 1

/* AMQP network layer specific internal data */
typedef struct {
    UA_NetworkAddressUrlDataType address;
    UA_Connection              *ua_connection;
    pn_connection_driver_t     *driver;
    //pn_connector_t  * connector;
    //pn_connector_t  * driver_connector;

    /* Moved connection_driver_t */
//    pn_connection_t * connection;
//    pn_collector_t  * collector;
    UA_Boolean      openLink;
    /* Total 2 links, 1 link can either sender or receiver */
    pn_link_t       * links [ MAX_AMQP_LINKS ];
    pn_message_t    * message;
    pn_rwbytes_t    message_buffer;
    pn_session_t    * session;
    pn_event_t      * event;
    pn_delivery_t   * delivery;
    UA_UInt32       sequence_no;
    UA_UInt32       acknowledged_no;

    UA_Boolean      writeDone;
    const char*     current;
    UA_Int32        remainingBytes;

} UA_AmqpContext;

UA_PubSubTransportLayer
UA_PubSubTransportLayerAMQP(void);



#endif //OPEN62541_UA_PUBSUB_AMQP_H
