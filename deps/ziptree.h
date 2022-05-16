/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018, 2021 (c) Julius Pfrommer
 */

#ifndef	ZIPTREE_H_
#define	ZIPTREE_H_

#include <stddef.h>

#ifdef _MSC_VER
# define ZIP_INLINE __inline
#else
# define ZIP_INLINE inline
#endif

/* Prevent warnings on unused static inline functions for some compilers */
#if defined(__GNUC__) || defined(__clang__)
# define ZIP_UNUSED __attribute__((unused))
#else
# define ZIP_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Reusable zip tree implementation. The style is inspired by the BSD
 * sys/queue.h linked list definition.
 *
 * Zip trees were developed in: Tarjan, R. E., Levy, C. C., and Timmel, S. "Zip
 * Trees." arXiv preprint arXiv:1806.06726 (2018). The original definition was
 * modified so that several elements with the same key can be inserted. However,
 * ZIP_FIND will only return the topmost of these elements in the tree.
 *
 * The ZIP_ENTRY definitions are to be contained in the tree entries themselves.
 * Use ZIP_FUNCTIONS to define the signature of the zip tree functions. */

#define ZIP_HEAD(name, type)                    \
struct name {                                   \
    struct type *root;                          \
}

#define ZIP_ENTRY(type)                         \
struct {                                        \
    struct type *left;                          \
    struct type *right;                         \
    unsigned char rank;                         \
}

enum ZIP_CMP {
    ZIP_CMP_LESS = -1,
    ZIP_CMP_EQ = 0,
    ZIP_CMP_MORE = 1
};

typedef enum ZIP_CMP (*zip_cmp_cb)(const void *key1, const void *key2);

#define ZIP_INIT(head) do { (head)->root = NULL; } while (0)
#define ZIP_ROOT(head) (head)->root
#define ZIP_LEFT(elm, field) (elm)->field.left
#define ZIP_RIGHT(elm, field) (elm)->field.right
#define ZIP_RANK(elm, field) (elm)->field.rank

/* Internal definitions. Don't use directly. */

typedef void (*__zip_iter_cb)(void *elm, void *context);

void *
__ZIP_INSERT(zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *root, void *elm);

void *
__ZIP_REMOVE(zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *root, void *elm);

void *
__ZIP_FIND(zip_cmp_cb cmp, unsigned short fieldoffset,
           unsigned short keyoffset, void *root,
           const void *key);

void
__ZIP_ITER(unsigned short fieldoffset, __zip_iter_cb cb,
           void *context, void *elm);

void * __ZIP_MIN(unsigned short fieldoffset, void *elm);
void * __ZIP_MAX(unsigned short fieldoffset, void *elm);

/* Zip trees are a probabilistic data structure. Entries are assigned a
 * (non-negative) rank k with probability 1/2^{k+1}. A uniformly sampled random
 * number has to be supplied with the insert method. __ZIP_FFS32 extracts from
 * it least significant nonzero bit of a 32bit number. This then has the correct
 * distribution. */
unsigned char __ZIP_FFS32(unsigned int v);

/* Generate zip tree method definitions with the ZIP_FUNCTIONS macro. The
 * comparison method "cmp" defined for every zip tree has the signature
 *
 *   enum ZIP_CMP cmpDateTime(const keytype *a, const keytype *b); */

#define ZIP_INSERT(name, head, elm, rank) name##_ZIP_INSERT(head, elm, rank)
#define ZIP_REMOVE(name, head, elm) name##_ZIP_REMOVE(head, elm)
#define ZIP_FIND(name, head, key) name##_ZIP_FIND(head, key)
#define ZIP_MIN(name, head) name##_ZIP_MIN(head)
#define ZIP_MAX(name, head) name##_ZIP_MAX(head)
#define ZIP_ITER(name, head, cb, d) name##_ZIP_ITER(head, cb, d)

#define ZIP_FUNCTIONS(name, type, field, keytype, keyfield, cmp)        \
ZIP_UNUSED static ZIP_INLINE void                                       \
name##_ZIP_INSERT(struct name *head, struct type *elm,                  \
                  unsigned int r) {                                     \
    ZIP_RANK(elm, field) = __ZIP_FFS32(r);                              \
    ZIP_ROOT(head) = (struct type*)                                     \
        __ZIP_INSERT(cmp, offsetof(struct type, field),                 \
                     offsetof(struct type, keyfield),                   \
                     ZIP_ROOT(head), elm);                              \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void                                       \
name##_ZIP_REMOVE(struct name *head, struct type *elm) {                \
    ZIP_ROOT(head) = (struct type*)                                     \
        __ZIP_REMOVE(cmp, offsetof(struct type, field),                 \
                     offsetof(struct type, keyfield),                   \
                     ZIP_ROOT(head), elm);                              \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_FIND(struct name *head, const keytype *key) {                \
    return (struct type*)__ZIP_FIND(cmp, offsetof(struct type, field),  \
                                    offsetof(struct type, keyfield),    \
                                    ZIP_ROOT(head), key);               \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_MIN(struct name *head) {                                     \
    return (struct type *)__ZIP_MIN(offsetof(struct type, field),       \
                                    ZIP_ROOT(head));                    \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_MAX(struct name *head) {                                     \
    return (struct type *)__ZIP_MAX(offsetof(struct type, field),       \
                                    ZIP_ROOT(head));                    \
}                                                                       \
                                                                        \
typedef void (*name##_cb)(struct type *elm, void *context);             \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void                                       \
name##_ZIP_ITER(struct name *head, name##_cb cb, void *context) {       \
    __ZIP_ITER(offsetof(struct type, field), (__zip_iter_cb)cb,         \
               context, ZIP_ROOT(head));                                \
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIPTREE_H_ */
