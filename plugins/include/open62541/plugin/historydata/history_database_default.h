/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#ifndef UA_HISTORYDATASERVICE_DEFAULT_H_
#define UA_HISTORYDATASERVICE_DEFAULT_H_

#include <open62541/plugin/historydatabase.h>

#include "history_data_gathering.h"

_UA_BEGIN_DECLS

UA_HistoryDatabase UA_EXPORT
UA_HistoryDatabase_default(UA_HistoryDataGathering gathering);

_UA_END_DECLS

#endif /* UA_HISTORYDATASERVICE_DEFAULT_H_ */
