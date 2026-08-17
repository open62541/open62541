/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "../common/interop_common.h"

int
main(int argc, char *argv[]) {
    return interopServerMain(argc, argv, UA_INTEROP_TRANSPORT_WEBSOCKET);
}
