/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <stddef.h>

/* The following uses __thread, which is available for clang.
 * No other compilers are currently used for fuzzing. */
extern __thread void * (*UA_mallocSingleton)(size_t size);
extern __thread void (*UA_freeSingleton)(void *ptr);
extern __thread void * (*UA_callocSingleton)(size_t nelem, size_t elsize);
extern __thread void * (*UA_reallocSingleton)(void *ptr, size_t size);

/* Define these and prevent the use of defaults in the open62541/config.h. */
#define UA_malloc(size) UA_mallocSingleton(size)
#define UA_free(ptr) UA_freeSingleton(ptr)
#define UA_calloc(num, size) UA_callocSingleton(num, size)
#define UA_realloc(ptr, size) UA_reallocSingleton(ptr, size)
