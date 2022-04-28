/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 */

#ifndef UA_NETWORK_PUBSUB_ETHERNET_H_
#define UA_NETWORK_PUBSUB_ETHERNET_H_

#include <open62541/plugin/pubsub.h>

_UA_BEGIN_DECLS

/**
 * EthernetWriterGroupTransportDataType
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */
/* TODO: This can be removed once publishingOffset parameter set in place
 * and normal DatagramWriterGroupTransportDataType can be used */
typedef struct {
    UA_Byte messageRepeatCount;
    UA_Double messageRepeatDelay;
    /* ETF related param - txtime to be published */
    UA_UInt64  transmission_time;
} UA_EthernetWriterGroupTransportDataType;

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerEthernet(void);

_UA_END_DECLS

#endif /* UA_NETWORK_PUBSUB_ETHERNET_H_ */
