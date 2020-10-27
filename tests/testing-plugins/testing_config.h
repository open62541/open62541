/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_CONFIG_H_
#define TESTING_CONFIG_H_

#include <open62541/server.h>

/* Creates a default config for testing. Currently this reduces the log level.
 * Because logging is limited in the CI pipeline. */
UA_Server * UA_Server_new_testing(void);

#endif /* TESTING_CONFIG_H_ */
