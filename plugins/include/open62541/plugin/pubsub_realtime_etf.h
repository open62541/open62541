/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#ifndef PUBSUB_REALTIME_ETF_H_
#define PUBSUB_REALTIME_ETF_H_

#ifdef UA_ENABLE_PUBSUB_REALTIME_PUBLISH_ETF

#include "ua_pubsub.h"
#include <open62541/plugin/pubsub_udp.h>
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#define   PUBSUB_IP_ADDRESS                               "192.168.8.105"
/* TODO: Set SO_PRIORITY through command line argument from application instead of hardcoding */
#define   SOCKET_PRIORITY                                 3

#ifndef   SOCKET_TRANSMISSION_TIME
#define   SOCKET_TRANSMISSION_TIME                        61
#ifndef   SCM_TXTIME
#define   SCM_TXTIME                                      SOCKET_TRANSMISSION_TIME
#endif
#endif

#ifndef   SOCKET_EE_ORIGIN_TRANSMISSION_TIME
#define   SOCKET_EE_ORIGIN_TRANSMISSION_TIME              6
#define   SOCKET_EE_CODE_TRANSMISSION_TIME_INVALID_PARAM  1
#define   SOCKET_EE_CODE_TRANSMISSION_TIME_MISSED         2
#endif

#define   TIMEOUT_REALTIME                                1

typedef struct {
    clockid_t clockIdentity;
    uint16_t flags;
} socket_transmission_time;

enum transmission_time_flags {
    SOF_TRANSMISSION_TIME_DEADLINE_MODE = (1 << 0),
    SOF_TRANSMISSION_TIME_REPORT_ERRORS = (1 << 1),
    SOF_TRANSMISSION_TIME_FLAGS_LAST = SOF_TRANSMISSION_TIME_REPORT_ERRORS,
    SOF_TRANSMISSION_TIME_FLAGS_MASK = (SOF_TRANSMISSION_TIME_FLAGS_LAST - 1) |
                 SOF_TRANSMISSION_TIME_FLAGS_LAST
};

/* Function for real time pubsub - txtime calculation*/
UA_StatusCode
txtimecalc_udp(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettigns, const UA_ByteString *buf);

/* Function for real time pubsub - txtime calculation*/
UA_StatusCode
txtimecalc_ethernet(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettigns, void *bufSend, int lenBuf);
#endif

#endif /* PUBSUB_REALTIME_ETF_H_ */
