/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef UA_EVENTLOOP_GLIB_H_
#define UA_EVENTLOOP_GLIB_H_

#include "eventloop_posix.h"

#if defined(UA_ENABLE_EVENTLOOP_GLIB) && \
    ((defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)) || \
     defined(UA_ARCHITECTURE_WIN32))

#include <glib.h>

_UA_BEGIN_DECLS

/* Cast a GPollFD* stored in UA_RegisteredFD::glibPollFD */
#define UA_RegisteredFD_glibPollFD(rfd) ((GPollFD*)(rfd)->glibPollFD)

_UA_END_DECLS

#endif /* UA_ENABLE_EVENTLOOP_GLIB && (POSIX || WIN32) */

#endif /* UA_EVENTLOOP_GLIB_H_ */
