/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef OPEN62541_CUSTOM_MEMORY_MANAGER_H
#define OPEN62541_CUSTOM_MEMORY_MANAGER_H

#include <open62541/types.h>

_UA_BEGIN_DECLS

/**
 * Set memory limit for memory manager.
 * This allows to reduce the available memory (RAM) for fuzzing tests.
 *
 * @param maxMemory Available memory in bytes
 */
void UA_EXPORT UA_memoryManager_setLimit(unsigned long long maxMemory);

/**
 * Extract the memory limit from the last four bytes of the byte array.
 * The last four bytes will simply be casted to a uint32_t and that value
 * represents the new memory limit.
 *
 * @param data byte array
 * @param size size of the byte array
 * @return 1 on success, 0 if the byte array is too short
 */
int UA_EXPORT UA_memoryManager_setLimitFromLast4Bytes(const uint8_t *data, size_t size);

_UA_END_DECLS

#endif /* OPEN62541_CUSTOM_MEMORY_MANAGER_H */
