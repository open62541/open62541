/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_config.h"

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

UA_Server *
UA_Server_new_testing() {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    config.logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_WARNING);
    UA_ServerConfig_setDefault(&config);
    return UA_Server_newWithConfig(&config);
}
