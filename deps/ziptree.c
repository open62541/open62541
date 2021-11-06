/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2021 (c) Julius Pfrommer
 */

#include "ziptree.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

unsigned char
ZIP_FFS32(unsigned int v) {
#if defined(__GNUC__) || defined(__clang__) 
    return ((unsigned char)__builtin_ffs((int)v)) - 1;
#elif defined(_MSC_VER)
    unsigned long index = 255;
    _BitScanForward(&index, v);
    return (unsigned char)index;
#else
    if(v == 0)
        return 255;
    unsigned int t = 1;
    unsigned char r = 0;
    while((v & t) == 0) {
        t = t << 1;
        r++;
    }
    return r;
#endif
}
