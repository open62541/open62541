/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_HISTORYDATASERVICE_DEFAULT_H_
#define UA_HISTORYDATASERVICE_DEFAULT_H_

#include "ua_server.h"
#include "ua_plugin_history_data_service.h"
#include "ua_plugin_history_data_gathering.h"

#ifdef __cplusplus
extern "C" {
#endif


UA_HistoryDataService UA_EXPORT
UA_HistoryDataService_Default(UA_HistoryDataGathering gathering);

#ifdef __cplusplus
}
#endif

#endif /* UA_HISTORYDATASERVICE_DEFAULT_H_ */
