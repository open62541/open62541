/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 *    Copyright 2021 (c) luibass92 <luibass92@live.it> (Author: Luigi Bassetta)
 */

#ifndef UA_HISTORYDATABACKEND_MEMORY_H_
#define UA_HISTORYDATABACKEND_MEMORY_H_

#include "history_data_backend.h"

_UA_BEGIN_DECLS

#define INITIAL_MEMORY_STORE_SIZE 1000

UA_HistoryDataBackend UA_EXPORT
UA_HistoryDataBackend_Memory(size_t initialNodeIdStoreSize, size_t initialDataStoreSize);

/* This function construct a UA_HistoryDataBackend which implements a circular buffer in memory.
 *
 * initialNodeIdStoreSize is the maximum number of NodeIds that will be historized. This number cannot be overcomed.
 * initialDataStoreSize is the maximum number of UA_DataValueMemoryStoreItem that will be saved in the circular buffer for a particular NodeId.
 *                      Subsequent UA_DataValueMemoryStoreItem will be saved replacing the oldest ones following the logic of circular buffers.
 */
UA_HistoryDataBackend UA_EXPORT
UA_HistoryDataBackend_Memory_Circular(size_t initialNodeIdStoreSize, size_t initialDataStoreSize);

void UA_EXPORT
UA_HistoryDataBackend_Memory_clear(UA_HistoryDataBackend *backend);

_UA_END_DECLS

#endif /* UA_HISTORYDATABACKEND_MEMORY_H_ */
