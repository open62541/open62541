/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_EVENTLOOP_LIBWEBSOCKETS_H_
#define UA_EVENTLOOP_LIBWEBSOCKETS_H_

#include "eventloop_posix.h"

#define _XOPEN_SOURCE 700
#include <libwebsockets.h>

/* one of these is appended to each pt for our use */
struct pt_eventlibs_custom {
    UA_POSIXConnectionManager cm;
    UA_EventLoopPOSIX *io_loop;
    struct lws_context *context;
};

extern const lws_plugin_evlib_t evlib_open62541;
extern lws_log_cx_t open62541_log_cx;

#endif /* UA_EVENTLOOP_LIBWEBSOCKETS_H_ */
