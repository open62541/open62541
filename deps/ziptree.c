/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2021-2022 (c) Julius Pfrommer
 */

#include "ziptree.h"

/* Dummy types */
struct zip_elem;
typedef struct zip_elem zip_elem;
typedef ZIP_ENTRY(zip_elem) zip_entry;
typedef ZIP_HEAD(, zip_elem) zip_head;

/* Access macros */
#define ZIP_ENTRY_PTR(x) ((zip_entry*)((char*)x + fieldoffset))
#define ZIP_KEY_PTR(x) (const void*)((const char*)x + keyoffset)

/* Hash pointers to keep the tie-breeaking of equal keys (mostly) uncorrelated
 * from the rank (pointer order). Hashing code taken from sdbm-hash
 * (http://www.cse.yorku.ca/~oz/hash.html). */
static unsigned int
__ZIP_PTR_HASH(const void *p) {
    unsigned int h = 0;
    const unsigned char *data = (const unsigned char*)&p;
    for(size_t i = 0; i < (sizeof(void*) / sizeof(char)); i++)
        h = data[i] + (h << 6) + (h << 16) - h;
    return h;
}

static ZIP_INLINE enum ZIP_CMP
__ZIP_RANK_CMP(const void *p1, const void *p2) {
    /* assert(p1 != p2); */
    unsigned int h1 = __ZIP_PTR_HASH(p1);
    unsigned int h2 = __ZIP_PTR_HASH(p2);
    if(h1 == h2)
        return (p1 < p2) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
    return (h1 < h2) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

static ZIP_INLINE enum ZIP_CMP
__ZIP_UNIQUE_CMP(zip_cmp_cb cmp, const void *p1, const void *p2) {
    if(p1 == p2)
        return ZIP_CMP_EQ;
    enum ZIP_CMP order = cmp(p1, p2);
    if(order == ZIP_CMP_EQ)
        return (p1 < p2) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
    return order;
}

#if 0
#include <assert.h>
ZIP_UNUSED static ZIP_INLINE void
__ZIP_VALIDATE(zip_cmp_cb cmp, unsigned short fieldoffset,
               unsigned short keyoffset, void *elm,
               void *min_elm, void *max_elm) {
    if(!elm)
        return;
    enum ZIP_CMP c1 = __ZIP_UNIQUE_CMP(cmp, ZIP_KEY_PTR(min_elm), ZIP_KEY_PTR(elm));
    assert((elm == min_elm && c1 == ZIP_CMP_EQ) || c1 == ZIP_CMP_LESS);

    enum ZIP_CMP c2 = __ZIP_UNIQUE_CMP(cmp, ZIP_KEY_PTR(max_elm), ZIP_KEY_PTR(elm));
    assert((elm == max_elm && c2 == ZIP_CMP_EQ) || c2 == ZIP_CMP_MORE);

    assert(!ZIP_ENTRY_PTR(elm)->right ||
           __ZIP_RANK_CMP(elm, ZIP_ENTRY_PTR(elm)->right) == ZIP_CMP_MORE);
    assert(!ZIP_ENTRY_PTR(elm)->left ||
           __ZIP_RANK_CMP(elm, ZIP_ENTRY_PTR(elm)->left) == ZIP_CMP_MORE);

    __ZIP_VALIDATE(cmp, fieldoffset, keyoffset, ZIP_ENTRY_PTR(elm)->right, elm, max_elm);
    __ZIP_VALIDATE(cmp, fieldoffset, keyoffset, ZIP_ENTRY_PTR(elm)->left, min_elm, elm);
}
#endif

/* Walk down the right-side spine of cur. Elements that are larger than x_key
 * are moved under x->right. */
static void
__ZIP_INSERT_MOVE_RIGHT(zip_cmp_cb cmp, unsigned short fieldoffset,
                        unsigned short keyoffset, const void *x_key,
                        zip_elem **fix_edge, zip_elem *cur) {
    while(ZIP_ENTRY_PTR(cur)->right) {
        zip_elem *move_candidate = ZIP_ENTRY_PTR(cur)->right;
        if(__ZIP_UNIQUE_CMP(cmp, x_key, ZIP_KEY_PTR(move_candidate)) == ZIP_CMP_MORE) {
            cur = ZIP_ENTRY_PTR(cur)->right;
            continue;
        }
        ZIP_ENTRY_PTR(cur)->right = ZIP_ENTRY_PTR(move_candidate)->left;
        ZIP_ENTRY_PTR(move_candidate)->left = NULL;
        *fix_edge = move_candidate;
        fix_edge = &ZIP_ENTRY_PTR(move_candidate)->left;
    }
}

static void
__ZIP_INSERT_MOVE_LEFT(zip_cmp_cb cmp, unsigned short fieldoffset,
                       unsigned short keyoffset, const void *x_key,
                       zip_elem **fix_edge, zip_elem *cur) {
    while(ZIP_ENTRY_PTR(cur)->left) {
        zip_elem *move_candidate = ZIP_ENTRY_PTR(cur)->left;
        if(__ZIP_UNIQUE_CMP(cmp, x_key, ZIP_KEY_PTR(move_candidate)) == ZIP_CMP_LESS) {
            cur = ZIP_ENTRY_PTR(cur)->left;
            continue;
        }
        ZIP_ENTRY_PTR(cur)->left = ZIP_ENTRY_PTR(move_candidate)->right;
        ZIP_ENTRY_PTR(move_candidate)->right = NULL;
        *fix_edge = move_candidate;
        fix_edge = &ZIP_ENTRY_PTR(move_candidate)->right;
    }
}

void
__ZIP_INSERT(void *h, zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *elm) {
    zip_elem *x = (zip_elem*)elm;
    ZIP_ENTRY_PTR(x)->left = NULL;
    ZIP_ENTRY_PTR(x)->right = NULL;

    const void *x_key = ZIP_KEY_PTR(x);
    zip_head *head = (zip_head*)h;
    if(!head->root) {
        head->root = x;
        return;
    }

    /* Go down the tree to find the top element "cur" that has a rank smaller
     * than "x" */
    zip_elem *prev = NULL;
    zip_elem *cur = head->root;
    enum ZIP_CMP cur_order, prev_order;
    do {
        cur_order = __ZIP_UNIQUE_CMP(cmp, x_key, ZIP_KEY_PTR(cur));
        if(cur_order == ZIP_CMP_EQ)
            return; /* x is already inserted */
        if(__ZIP_RANK_CMP(cur, x) == ZIP_CMP_LESS)
            break;
        prev = cur;
        prev_order = cur_order;
        cur = (cur_order == ZIP_CMP_MORE) ?
            ZIP_ENTRY_PTR(cur)->right : ZIP_ENTRY_PTR(cur)->left;
    } while(cur);

    /* Insert "x" instead of "cur" under its parent "prev" */
    if(cur == head->root) {
        head->root = x;
    } else {
        if(prev_order == ZIP_CMP_MORE)
            ZIP_ENTRY_PTR(prev)->right = x;
        else
            ZIP_ENTRY_PTR(prev)->left = x;
    }

    if(!cur)
        return;

    /* Re-insert "cur" under "x". Repair by moving elements that ended up on the
     * wrong side of "x". */
    if(cur_order == ZIP_CMP_MORE) {
        ZIP_ENTRY_PTR(x)->left = cur;
        __ZIP_INSERT_MOVE_RIGHT(cmp, fieldoffset, keyoffset,
                                x_key, &ZIP_ENTRY_PTR(x)->right, cur);
    } else {
        ZIP_ENTRY_PTR(x)->right = cur;
        __ZIP_INSERT_MOVE_LEFT(cmp, fieldoffset, keyoffset,
                               x_key, &ZIP_ENTRY_PTR(x)->left, cur);
    }
}

void *
__ZIP_REMOVE(void *h, zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *elm) {
    zip_head *head = (zip_head*)h;
    zip_elem *x = (zip_elem*)elm;
    zip_elem *cur = head->root;
    if(!cur)
        return NULL;

    const void *x_key = ZIP_KEY_PTR(x);
    zip_elem **prev_edge = &head->root;
    enum ZIP_CMP cur_order = __ZIP_UNIQUE_CMP(cmp, x_key, ZIP_KEY_PTR(cur));
    while(cur_order != ZIP_CMP_EQ) {
        prev_edge = (cur_order == ZIP_CMP_LESS) ?
            &ZIP_ENTRY_PTR(cur)->left : &ZIP_ENTRY_PTR(cur)->right;
        cur = *prev_edge;
        if(!cur)
            return NULL;
        cur_order = __ZIP_UNIQUE_CMP(cmp, x_key, ZIP_KEY_PTR(cur));
    }
    *prev_edge = (zip_elem*)__ZIP_ZIP(fieldoffset,
                                      ZIP_ENTRY_PTR(cur)->left,
                                      ZIP_ENTRY_PTR(cur)->right);
    return cur;
}

void *
__ZIP_ITER(unsigned short fieldoffset, zip_iter_cb cb,
           void *context, void *elm) {
    if(!elm)
        return NULL;
    zip_elem *left = ZIP_ENTRY_PTR(elm)->left;
    zip_elem *right = ZIP_ENTRY_PTR(elm)->right;
    void *res = __ZIP_ITER(fieldoffset, cb, context, left);
    if(res)
        return res;
    res = cb(context, elm);
    if(res)
        return res;
    return __ZIP_ITER(fieldoffset, cb, context, right);
}

void *
__ZIP_ITER_KEY(zip_cmp_cb cmp, unsigned short fieldoffset,
               unsigned short keyoffset, const void *key,
               zip_iter_cb cb, void *context, void *elm) {
    if(!elm)
        return NULL;

    void *res;
    enum ZIP_CMP eq = cmp(key, ZIP_KEY_PTR(elm));
    if(eq != ZIP_CMP_MORE) {
        res = __ZIP_ITER_KEY(cmp, fieldoffset, keyoffset, key,
                             cb, context, ZIP_ENTRY_PTR(elm)->left);
        if(res)
            return res;
    }

    if(eq == ZIP_CMP_EQ) {
        res = cb(context, elm);
        if(res)
            return res;
    }

    if(eq != ZIP_CMP_LESS) {
        res = __ZIP_ITER_KEY(cmp, fieldoffset, keyoffset, key,
                             cb, context, ZIP_ENTRY_PTR(elm)->right);
        if(res)
            return res;
    }

    return NULL;
}

void *
__ZIP_ZIP(unsigned short fieldoffset, void *left, void *right) {
    if(!left)
        return right;
    if(!right)
        return left;
    zip_elem *l = (zip_elem*)left;
    zip_elem *r = (zip_elem*)right;
    zip_elem *root = NULL;
    zip_elem **prev_edge = &root;
    while(l && r) {
        if(__ZIP_RANK_CMP(l, r) == ZIP_CMP_LESS) {
            *prev_edge = r;
            prev_edge = &ZIP_ENTRY_PTR(r)->left;
            r = ZIP_ENTRY_PTR(r)->left;
        } else {
            *prev_edge = l;
            prev_edge = &ZIP_ENTRY_PTR(l)->right;
            l = ZIP_ENTRY_PTR(l)->right;
        }
    }
    *prev_edge = (l) ? l : r;
    return root;
}

/* Walk down from cur and move all elements <= split-key to the left side. All
 * elements that are moved over have to be below left_rightmost. Returns the
 * hierarchy of elements that remain on the right side. */
static void
__ZIP_UNZIP_MOVE_LEFT(zip_cmp_cb cmp, unsigned short fieldoffset,
                      unsigned short keyoffset, const void *key,
                      zip_elem **fix_edge, zip_elem *cur) {
    while(ZIP_ENTRY_PTR(cur)->left) {
        zip_elem *next = ZIP_ENTRY_PTR(cur)->left;
        if(cmp(key, ZIP_KEY_PTR(next)) == ZIP_CMP_LESS) {
            cur = next;
            continue;
        }
        *fix_edge = next;
        ZIP_ENTRY_PTR(cur)->left = ZIP_ENTRY_PTR(next)->right;
        ZIP_ENTRY_PTR(next)->right = NULL;
        fix_edge = &ZIP_ENTRY_PTR(next)->right;
    }
}

static void
__ZIP_UNZIP_MOVE_RIGHT(zip_cmp_cb cmp, unsigned short fieldoffset,
                       unsigned short keyoffset, const void *key,
                       zip_elem **fix_edge, zip_elem *cur) {
    while(ZIP_ENTRY_PTR(cur)->right) {
        zip_elem *next = ZIP_ENTRY_PTR(cur)->right;
        if(cmp(key, ZIP_KEY_PTR(next)) != ZIP_CMP_LESS) {
            cur = next;
            continue;
        }
        *fix_edge = next;
        ZIP_ENTRY_PTR(cur)->right = ZIP_ENTRY_PTR(next)->left;
        ZIP_ENTRY_PTR(next)->left = NULL;
        fix_edge = &ZIP_ENTRY_PTR(next)->left;
    }
}

/* Split the tree into a left side with keys <= split-key and a right side with
 * key > split-key. */
void
__ZIP_UNZIP(zip_cmp_cb cmp, unsigned short fieldoffset,
            unsigned short keyoffset, const void *key,
            void *h, void *l, void *r) {
    zip_elem *prev;
    zip_head *head = (zip_head*)h;
    zip_head *left = (zip_head*)l;
    zip_head *right = (zip_head*)r;
    if(!head->root) {
        left->root = NULL;
        right->root = NULL;
        return;
    }
    zip_elem *cur = head->root;
    if(cmp(key, ZIP_KEY_PTR(cur)) != ZIP_CMP_LESS) {
        left->root = cur;
        do {
            prev = cur;
            cur = ZIP_ENTRY_PTR(cur)->right;
            if(!cur) {
                right->root = NULL;
                return;
            }
        } while(cmp(key, ZIP_KEY_PTR(cur)) != ZIP_CMP_LESS);
        ZIP_ENTRY_PTR(prev)->right = NULL;
        right->root = cur;
        __ZIP_UNZIP_MOVE_LEFT(cmp, fieldoffset, keyoffset, key,
                              &ZIP_ENTRY_PTR(prev)->right, cur);
    } else {
        right->root = cur;
        do {
            prev = cur;
            cur = ZIP_ENTRY_PTR(cur)->left;
            if(!cur) {
                left->root = NULL;
                return;
            }
        } while(cmp(key, ZIP_KEY_PTR(cur)) == ZIP_CMP_LESS);
        ZIP_ENTRY_PTR(prev)->left = NULL;
        left->root = cur;
        __ZIP_UNZIP_MOVE_RIGHT(cmp, fieldoffset, keyoffset, key,
                               &ZIP_ENTRY_PTR(prev)->left, cur);
    }
}
