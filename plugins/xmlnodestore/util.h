/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */
#define _POSIX_C_SOURCE 199309L
#ifndef UTIL_H
#define UTIL_H
#include <assert.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#define assertf(A, M)                                                                    \
    if(!A) {                                                                             \
        printf(M);                                                                       \
        assert(A);                                                                       \
    }

bool strEqual(const char *lhs, const char *rhs);

struct timespec diff(struct timespec start, struct timespec end);

#endif