/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_CHECK_COMPAT_H_
#define UA_CHECK_COMPAT_H_

/* Set architecture feature macros before Check includes any system headers. */
#include <open62541/config.h>
#include <check.h>
#include <string.h>

/* Keep tests usable with older Check releases shipped by CI toolchains. */
#ifndef ck_assert_ptr_nonnull
#define ck_assert_ptr_nonnull(X) ck_assert_ptr_ne((X), NULL)
#endif
#ifndef ck_assert_mem_eq
#define ck_assert_mem_eq(X, Y, L) ck_assert(memcmp((X), (Y), (L)) == 0)
#endif
#ifndef ck_assert_float_eq
#define ck_assert_float_eq(X, Y) ck_assert((float)(X) == (float)(Y))
#endif

#endif /* UA_CHECK_COMPAT_H_ */
