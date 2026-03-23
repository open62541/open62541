/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Moritz Bruder)
 */

#ifndef UA_CERTIFICATEGROUP_COMMON_H_
#define UA_CERTIFICATEGROUP_COMMON_H_

#define UA_SPLITSTATUSCODE_HIDDEN(result) \
    ((UA_SplitStatusCode){(result), UA_STATUSCODE_BADSECURITYCHECKSFAILED})

#endif /* UA_CERTIFICATEGROUP_COMMON_H_ */
