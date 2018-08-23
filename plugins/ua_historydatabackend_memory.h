/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_HISTORYDATABACKEND_MEMORY_H_
#define UA_HISTORYDATABACKEND_MEMORY_H_

#include "ua_plugin_history_data_backend.h"

#define INITIAL_MEMORY_STORE_SIZE 1000

#ifdef __cplusplus
extern "C" {
#endif


UA_HistoryDataBackend UA_EXPORT
UA_HistoryDataBackend_Memory(size_t initialNodeIdStoreSize, size_t initialDataStoreSize);

void UA_EXPORT
UA_HistoryDataBackend_Memory_deleteMembers(UA_HistoryDataBackend *backend);

#ifdef __cplusplus
}
#endif

#endif /* UA_HISTORYDATABACKEND_MEMORY_H_ */
