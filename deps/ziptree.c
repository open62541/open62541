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
__ZIP_FFS32(unsigned int v) {
#if defined(__GNUC__) || defined(__clang__) 
    return (unsigned char)((unsigned char)__builtin_ffs((int)v) - 1u);
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

/* Generic analog to ZIP_ENTRY(type) */
struct zip_entry {
    void *left;
    void *right;
    unsigned char rank;
};

#define ZIP_ENTRY_PTR(x) (struct zip_entry*)((char*)x + fieldoffset)
#define ZIP_KEY_PTR(x) (void*)((char*)x + keyoffset)

void *
__ZIP_INSERT(zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *root, void *elm) {
    struct zip_entry *elm_entry  = ZIP_ENTRY_PTR(elm);
    if(!root) {
        elm_entry->left = NULL;
        elm_entry->right = NULL;
        return elm;
    }
    struct zip_entry *root_entry = ZIP_ENTRY_PTR(root);
    enum ZIP_CMP order = cmp(ZIP_KEY_PTR(elm), ZIP_KEY_PTR(root));
    if(order == ZIP_CMP_LESS) {
        if(__ZIP_INSERT(cmp, fieldoffset, keyoffset, root_entry->left, elm) == elm) {
            if(elm_entry->rank < root_entry->rank) {
                root_entry->left = elm;
            } else {
                root_entry->left = elm_entry->right;
                elm_entry->right = root;
                return elm;
            }
        }
    } else {
        if(__ZIP_INSERT(cmp, fieldoffset, keyoffset, root_entry->right, elm) == elm) {
            if(elm_entry->rank <= root_entry->rank) {
                root_entry->right = elm;
            } else {
                root_entry->right = elm_entry->left;
                elm_entry->left = root;
                return elm;
            }
        }
    }
    return root;
}

static void *
__ZIP(unsigned short fieldoffset, void *x, void *y) {
    if(!x) return y;
    if(!y) return x;
    struct zip_entry *x_entry = ZIP_ENTRY_PTR(x);
    struct zip_entry *y_entry = ZIP_ENTRY_PTR(y);
    if(x_entry->rank < y_entry->rank) {
        y_entry->left = __ZIP(fieldoffset, x, y_entry->left);
        return y;
    } else {
        x_entry->right = __ZIP(fieldoffset, x_entry->right, y);
        return x;
    }
}

/* Modified from the original algorithm. Allow multiple elements with the same
 * key. */
void *
__ZIP_REMOVE(zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *root, void *elm) {
    struct zip_entry *root_entry = ZIP_ENTRY_PTR(root);
    if(root == elm)
        return __ZIP(fieldoffset, root_entry->left, root_entry->right);
    void *left = root_entry->left;
    void *right = root_entry->right;
    enum ZIP_CMP eq = cmp(ZIP_KEY_PTR(elm), ZIP_KEY_PTR(root));
    if(eq == ZIP_CMP_LESS) {
        struct zip_entry *left_entry = ZIP_ENTRY_PTR(left);
        if(elm == left)
            root_entry->left = __ZIP(fieldoffset, left_entry->left, left_entry->right);
        else if(left)
            __ZIP_REMOVE(cmp, fieldoffset, keyoffset, left, elm);
    } else if(eq == ZIP_CMP_MORE) {
        struct zip_entry *right_entry = ZIP_ENTRY_PTR(right);
        if(elm == right)
            root_entry->right = __ZIP(fieldoffset, right_entry->left, right_entry->right);
        else if(right)
            __ZIP_REMOVE(cmp, fieldoffset, keyoffset, right, elm);
    } else { /* ZIP_CMP_EQ, but root != elm */
        if(right)
            root_entry->right = __ZIP_REMOVE(cmp, fieldoffset, keyoffset, right, elm);
        if(left)
            root_entry->left = __ZIP_REMOVE(cmp, fieldoffset, keyoffset, left, elm);
    }
    return root;
}

void *
__ZIP_FIND(zip_cmp_cb cmp, unsigned short fieldoffset,
           unsigned short keyoffset, void *root, const void *key) {
    if(!root)
        return NULL;
    enum ZIP_CMP eq = cmp(key, ZIP_KEY_PTR(root));
    if(eq == ZIP_CMP_EQ)
        return root;
    struct zip_entry *root_entry = ZIP_ENTRY_PTR(root);
    if(eq == ZIP_CMP_LESS)
        return __ZIP_FIND(cmp, fieldoffset, keyoffset, root_entry->left, key);
    else
        return __ZIP_FIND(cmp, fieldoffset, keyoffset, root_entry->right, key);
}

void *
__ZIP_MIN(unsigned short fieldoffset, void *elm) {
    if(!elm)
        return NULL;
    struct zip_entry *elm_entry = ZIP_ENTRY_PTR(elm);
    while(elm_entry->left) {
        elm = elm_entry->left;
        elm_entry  = (struct zip_entry*)((char*)elm + fieldoffset);
    }
    return elm;
}

void *
__ZIP_MAX(unsigned short fieldoffset, void *elm) {
    if(!elm)
        return NULL;
    struct zip_entry *elm_entry = ZIP_ENTRY_PTR(elm);
    while(elm_entry->right) {
        elm = elm_entry->right;
        elm_entry  = (struct zip_entry*)((char*)elm + fieldoffset);
    }
    return elm;
}

void
__ZIP_ITER(unsigned short fieldoffset, __zip_iter_cb cb,
           void *context, void *elm) {
    if(!elm)
        return;
    struct zip_entry *elm_entry = ZIP_ENTRY_PTR(elm);
    __ZIP_ITER(fieldoffset, cb, context, elm_entry->left);
    __ZIP_ITER(fieldoffset, cb, context, elm_entry->right);
    cb(elm, context);
}
