/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_HISTORYDATAGATHERING_DEFAULT_H_
#define UA_HISTORYDATAGATHERING_DEFAULT_H_

#include "ua_plugin_history_data_gathering.h"

#ifdef __cplusplus
extern "C" {
#endif


UA_HistoryDataGathering UA_EXPORT
UA_HistoryDataGathering_Default(size_t initialNodeIdStoreSize);

#ifdef __cplusplus
}
#endif

#endif /* UA_HISTORYDATAGATHERING_DEFAULT_H_ */
