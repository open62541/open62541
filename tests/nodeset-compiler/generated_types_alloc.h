/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Inject one allocation failure at a time into the generated-type tests.
 * For a dedicated build, pass
 * -DCMAKE_C_FLAGS="-include /absolute/path/to/generated_types_alloc.h"
 * and build/run check_nodeset_copy_types. */
#ifndef GENERATED_TYPES_ALLOC_H_
#define GENERATED_TYPES_ALLOC_H_
#include <stdlib.h>
void *generatedTypesTestMalloc(size_t size);
void *generatedTypesTestCalloc(size_t count, size_t size);
void *generatedTypesTestRealloc(void *ptr, size_t size);
#define UA_malloc generatedTypesTestMalloc
#define UA_calloc generatedTypesTestCalloc
#define UA_realloc generatedTypesTestRealloc
#define UA_free free
#define UA_TEST_GENERATED_TYPES_ALLOCATION_FAILURES
#endif
