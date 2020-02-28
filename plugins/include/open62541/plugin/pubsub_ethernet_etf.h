/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#ifndef UA_NETWORK_PUBSUB_ETHERNET_ETF_H_
#define UA_NETWORK_PUBSUB_ETHERNET_ETF_H_

#include <open62541/plugin/pubsub.h>

_UA_BEGIN_DECLS

/**
 * EthernetETFWriterGroupTransportDataType
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */
typedef struct {
    UA_UInt64  transmission_time;
    UA_Boolean txtime_enabled;
    /* TODO: Transmission time flags */
} UA_EthernetETFWriterGroupTransportDataType;

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerEthernetETF(void);

_UA_END_DECLS

#endif /* UA_NETWORK_PUBSUB_ETHERNET_ETF_H_ */
