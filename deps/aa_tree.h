/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef	AA_TREE_H_
#define	AA_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum aa_cmp {
    AA_CMP_LESS = -1,
    AA_CMP_EQ = 0,
    AA_CMP_MORE = 1
};

struct aa_entry {
    struct aa_entry *left;
    struct aa_entry *right;
    unsigned int level;
};

struct aa_head {
    struct aa_entry *root;
    enum aa_cmp (*cmp)(const void* a, const void* b);
    /* Offset from the container element to the aa_entry and the key */
    unsigned int entry_offset;
    unsigned int key_offset;
};

/* The AA-Tree allows duplicate entries. The first matching key is returned in
 * aa_find. */

void aa_init(struct aa_head *head,
             enum aa_cmp (*cmp)(const void*, const void*),
             unsigned int entry_offset, unsigned int key_offset);
void aa_insert(struct aa_head *head, void *elem);
void aa_remove(struct aa_head *head, void *elem);
void * aa_find(const struct aa_head *head, const void *key);
void * aa_min(const struct aa_head *head);
void * aa_max(const struct aa_head *head);
void * aa_next(const struct aa_head *head, const void *elem);
void * aa_prev(const struct aa_head *head, const void *elem);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AA_TREE_H_ */
