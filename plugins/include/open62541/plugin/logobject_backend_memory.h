/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_LOGOBJECT_BACKEND_MEMORY_H_
#define UA_LOGOBJECT_BACKEND_MEMORY_H_

#include <open62541/plugin/logobject.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_LOGOBJECT

/* In-memory storage for LogObjects. Every registered LogObject gets a ring
 * buffer of settings.maxRecords records. The oldest record is evicted when the
 * ring is full (reported as overflow to the core). Records older than
 * settings.maxStorageDuration are dropped lazily whenever records are appended
 * or read. The cursor of this backend is the sequence number of a record.
 *
 * The backend is thread-safe: addRecord may be called concurrently from any
 * thread. */
UA_LogObjectBackend UA_EXPORT
UA_LogObjectBackend_Memory(void);

#endif /* UA_ENABLE_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_LOGOBJECT_BACKEND_MEMORY_H_ */
