/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#include "ua_server.h"

/** @brief Create the TCP networklayer and listen to the specified port */
UA_Connection createDummyConnection(void);

#endif /* TESTING_NETWORKLAYERS_H_ */
