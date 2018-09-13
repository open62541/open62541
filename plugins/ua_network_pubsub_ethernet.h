/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 */

#ifndef UA_NETWORK_PUBSUB_ETHERNET_H_
#define UA_NETWORK_PUBSUB_ETHERNET_H_

#include "ua_plugin_pubsub.h"

/* TransportProfileUri  */
/* The open62541 example indicates for UDP connection:
 *    "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
 * Not known what to set for this layer.
 * T.B.D.
 */
_UA_BEGIN_DECLS

#define TRANSPORT_PROFILE_URI_ETHERNET "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"

UA_PubSubTransportLayer UA_PubSubTransportLayerEthernet(void);

_UA_END_DECLS

#endif /* UA_NETWORK_PUBSUB_ETHERNET_H_ */
