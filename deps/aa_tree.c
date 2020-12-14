/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "aa_tree.h"
#include <stddef.h>

#if !defined(_MSC_VER) || _MSC_VER >= 1800
# include <inttypes.h>
#elif !defined(uintptr_t)
  /* Workaround missing standard includes in older Visual Studio */
# if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
typedef _W64 unsigned int uintptr_t;
# else
typedef unsigned __int64 uintptr_t;
# endif
#endif

#define aa_entry_container(head, entry)                 \
    ((void*)((uintptr_t)entry - head->entry_offset))
#define aa_entry_key(head, entry)                       \
    ((const void*)((uintptr_t)entry + head->key_offset - head->entry_offset))
#define aa_container_entry(head, container)             \
    ((struct aa_entry*)((uintptr_t)container + head->entry_offset))
#define aa_container_key(head, container)               \
    ((const void*)((uintptr_t)container + head->key_offset))

void
aa_init(struct aa_head *head,
        enum aa_cmp (*cmp)(const void*, const void*),
        unsigned int entry_offset, unsigned int key_offset) {
    head->root = NULL;
    head->cmp = cmp;
    head->entry_offset = entry_offset;
    head->key_offset = key_offset;
}

static struct aa_entry *
_aa_skew(struct aa_entry *n) {
    if(!n)
        return NULL;
    if(n->left && n->level == n->left->level) {
        struct aa_entry *l = n->left;
        n->left = l->right;
        l->right = n;
        return l;
    }
    return n;
}

static struct aa_entry *
_aa_split(struct aa_entry *n) {
    if(!n)
        return NULL;
    if(n->right && n->right->right &&
       n->right->right->level == n->level) {
        struct aa_entry *r = n->right;
        n->right = r->left;
        r->left = n;
        r->level++;
        return r;
    }
    return n;
}

static struct aa_entry *
_aa_fixup(struct aa_entry *n) {
    unsigned int should_be = 0;
    if(n->left)
        should_be = n->left->level;
    if(n->right && n->right->level < should_be)
        should_be = n->right->level;
    should_be++;
    if(should_be < n->level)
        n->level = should_be;
    if(n->right && n->right->level > should_be)
        n->right->level = should_be;
    n = _aa_skew(n);
    n->right = _aa_skew(n->right);
    if(n->right)
        n->right->right = _aa_skew(n->right->right);
    n = _aa_split(n);
    n->right = _aa_split(n->right);
    return n;
}

static struct aa_entry *
_aa_insert(struct aa_head *h, struct aa_entry *n, void *elem) {
    if(!n) {
        struct aa_entry *e = aa_container_entry(h, elem);
        e->left = NULL;
        e->right = NULL;
        e->level = 1;
        return e;
    }
    const void *n_key = aa_entry_key(h, n);
    const void *key = aa_container_key(h, elem);
    enum aa_cmp eq = h->cmp(key, n_key);
    if(eq == AA_CMP_EQ)
        eq = (key > n_key) ? AA_CMP_MORE : AA_CMP_LESS;
    if(eq == AA_CMP_LESS)
        n->left = _aa_insert(h, n->left, elem);
    else
        n->right = _aa_insert(h, n->right, elem);
    return _aa_split(_aa_skew(n));
}

void
aa_insert(struct aa_head *h, void *elem) {
    h->root = _aa_insert(h, h->root, elem);
}

static struct aa_entry *
_aa_find(const struct aa_head *h, struct aa_entry *n, const void *key) {
    if(!n)
        return NULL;
    enum aa_cmp eq = h->cmp(key, aa_entry_key(h, n));
    if(eq == AA_CMP_EQ)
        return n;
    if(eq == AA_CMP_LESS)
        return _aa_find(h, n->left, key);
    return _aa_find(h, n->right, key);
}

void *
aa_find(const struct aa_head *h, const void *key) {
    struct aa_entry *n = _aa_find(h, h->root, key);
    if(!n)
        return NULL;
    return aa_entry_container(h, n);
}

static struct aa_entry *
unlink_succ(struct aa_entry *n, struct aa_entry **succ) {
    if(!n->left) {
        *succ = n;
        return n->right;
    }
    n->left = unlink_succ(n->left, succ);
    return _aa_fixup(n);
}

static struct aa_entry *
unlink_pred(struct aa_entry *n, struct aa_entry **pred) {
    if(!n->right) {
        *pred = n;
        return n->left;
    }
    n->right = unlink_pred(n->right, pred);
    return _aa_fixup(n);
}

static struct aa_entry *
_aa_remove(struct aa_head *h, struct aa_entry *n, void *elem) {
    if(!n)
        return NULL;
    const void *n_key = aa_entry_key(h, n);
    const void *key = aa_container_key(h, elem);
    if(n_key != key) {
        enum aa_cmp eq = h->cmp(key, n_key);
        if(eq == AA_CMP_EQ)
            eq = (key > n_key) ? AA_CMP_MORE : AA_CMP_LESS;
        if(eq == AA_CMP_LESS)
            n->left = _aa_remove(h, n->left, elem);
        else
            n->right = _aa_remove(h, n->right, elem);
    } else {
        if(!n->left && !n->right)
            return NULL;
        struct aa_entry *replace = NULL;
        if(!n->left)
            n->right = unlink_succ(n->right, &replace);
        else
            n->left = unlink_pred(n->left, &replace);
        replace->left = n->left;
        replace->right = n->right;
        replace->level = n->level;
        n = replace;
    }
    return _aa_fixup(n);
}

void
aa_remove(struct aa_head *head, void *elem) {
    head->root = _aa_remove(head, head->root, elem);
}

void *
aa_min(const struct aa_head *head) {
    struct aa_entry *e = head->root;
    if(!e)
        return NULL;
    while(e->left)
        e = e->left;
    return aa_entry_container(head, e);
}

void *
aa_max(const struct aa_head *head) {
    struct aa_entry *e = head->root;
    if(!e)
        return NULL;
    while(e->right)
        e = e->right;
    return aa_entry_container(head, e);
}

void *
aa_next(const struct aa_head *head, const void *elem) {
    struct aa_entry *e = aa_container_entry(head, elem);
    if(e->right) {
        e = e->right;
        while(e->left)
            e = e->left;
        return aa_entry_container(head, e);
    }
    struct aa_entry *next = NULL;
    struct aa_entry *n = head->root;
    const void *key = aa_container_key(head, elem);
    while(n && n != e) {
        const void *n_key = aa_entry_key(head, n);
        enum aa_cmp eq = head->cmp(key, n_key);
        if(eq == AA_CMP_EQ)
            eq = (key > n_key) ? AA_CMP_MORE : AA_CMP_LESS;
        if(eq == AA_CMP_MORE) {
            n = n->right;
        } else {
            next = n;
            n = n->left;
        }
    }
    return (next) ? aa_entry_container(head, next) : NULL;
}

void *
aa_prev(const struct aa_head *head, const void *elem) {
    struct aa_entry *e = aa_container_entry(head, elem);
    if(e->left) {
        e = e->left;
        while(e->right)
            e = e->right;
        return aa_entry_container(head, e);
    }
    struct aa_entry *prev = NULL;
    struct aa_entry *n = head->root;
    const void *key = aa_container_key(head, elem);
    while(n && n != e) {
        const void *n_key = aa_entry_key(head, n);
        enum aa_cmp eq = head->cmp(key, n_key);
        if(eq == AA_CMP_EQ)
            eq = (key > n_key) ? AA_CMP_MORE : AA_CMP_LESS;
        if(eq == AA_CMP_MORE) {
            prev = n;
            n = n->right;
        } else {
            n = n->left;
        }
    }
    return (prev) ? aa_entry_container(head, prev) : NULL;
}
