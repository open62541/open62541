/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018, 2021-2022 (c) Julius Pfrommer
 */

#ifndef	ZIPTREE_H_
#define	ZIPTREE_H_

#include <stddef.h>

#ifdef _MSC_VER
# define ZIP_INLINE __inline
#else
# define ZIP_INLINE inline
#endif

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
 * modified in two ways:
 *
 * - Multiple elements with the same key can be inserted. These appear adjacent
 *   in the tree. ZIP_FIND will return the topmost of these elements.
 * - The pointer-value of the elements are used as the rank. This simplifies the
 *   code and is (empirically) faster.
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
}

enum ZIP_CMP {
    ZIP_CMP_LESS = -1,
    ZIP_CMP_EQ = 0,
    ZIP_CMP_MORE = 1
};

/* The comparison method "cmp" for a zip tree has the signature.
 * Provide this to the ZIP_FUNCTIONS macro.
 *
 *   enum ZIP_CMP cmpMethod(const keytype *a, const keytype *b);
 */
typedef enum ZIP_CMP (*zip_cmp_cb)(const void *key1, const void *key2);

#define ZIP_INIT(head) do { (head)->root = NULL; } while (0)
#define ZIP_ROOT(head) (head)->root
#define ZIP_LEFT(elm, field) (elm)->field.left
#define ZIP_RIGHT(elm, field) (elm)->field.right
#define ZIP_INSERT(name, head, elm) name##_ZIP_INSERT(head, elm)
#define ZIP_FIND(name, head, key) name##_ZIP_FIND(head, key)
#define ZIP_MIN(name, head) name##_ZIP_MIN(head)
#define ZIP_MAX(name, head) name##_ZIP_MAX(head)

/* Returns the element if it was found in the tree. Returns NULL otherwise. */
#define ZIP_REMOVE(name, head, elm) name##_ZIP_REMOVE(head, elm)

/* Split (_UNZIP) and merge (_ZIP) trees. _UNZIP splits at the key and moves
 * elements <= into the left output (right otherwise). */
#define ZIP_ZIP(name, left, right) name##_ZIP_ZIP(left, right)
#define ZIP_UNZIP(name, head, key, left, right) \
    name##_ZIP_UNZIP(head, key, left, right)

/* ZIP_ITER uses in-order traversal of the tree (in the order of the keys). The
 * memory if a node is not accessed by ZIP_ITER after the callback has been
 * executed for it. So a tree can be cleaned by calling free on each node from
 * within the iteration callback.
 *
 * ZIP_ITER returns a void pointer. The first callback to return non-NULL aborts
 * the iteration. This pointer is then returned. */
typedef void * (*zip_iter_cb)(void *context, void *elm);
#define ZIP_ITER(name, head, cb, ctx) name##_ZIP_ITER(head, cb, ctx)

/* Same as _ITER, but only visits elements with the given key */
#define ZIP_ITER_KEY(name, head, key, cb, ctx) name##_ZIP_ITER_KEY(head, key, cb, ctx)

/* Macro to generate typed ziptree methods */
#define ZIP_FUNCTIONS(name, type, field, keytype, keyfield, cmp)        \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void                                       \
name##_ZIP_INSERT(struct name *head, struct type *el) {                 \
    __ZIP_INSERT(head, (zip_cmp_cb)cmp, offsetof(struct type, field),   \
                 offsetof(struct type, keyfield), el);                  \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_REMOVE(struct name *head, struct type *elm) {                \
    return (struct type*)                                               \
        __ZIP_REMOVE(head, (zip_cmp_cb)cmp,                             \
                     offsetof(struct type, field),                      \
                     offsetof(struct type, keyfield), elm);             \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_FIND(struct name *head, const keytype *key) {                \
    struct type *cur = ZIP_ROOT(head);                                  \
    while(cur) {                                                        \
        enum ZIP_CMP eq = cmp(key, &cur->keyfield);                     \
        if(eq == ZIP_CMP_EQ)                                            \
            break;                                                      \
        if(eq == ZIP_CMP_LESS)                                          \
            cur = ZIP_LEFT(cur, field);                                 \
        else                                                            \
            cur = ZIP_RIGHT(cur, field);                                \
    }                                                                   \
    return cur;                                                         \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_MIN(struct name *head) {                                     \
    struct type *cur = ZIP_ROOT(head);                                  \
    if(!cur)                                                            \
        return NULL;                                                    \
    while(ZIP_LEFT(cur, field)) {                                       \
        cur = ZIP_LEFT(cur, field);                                     \
    }                                                                   \
    return cur;                                                         \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_MAX(struct name *head) {                                     \
    struct type *cur = ZIP_ROOT(head);                                  \
    if(!cur)                                                            \
        return NULL;                                                    \
    while(ZIP_RIGHT(cur, field)) {                                      \
        cur = ZIP_RIGHT(cur, field);                                    \
    }                                                                   \
    return cur;                                                         \
}                                                                       \
                                                                        \
typedef void * (*name##_cb)(void *context, struct type *elm);           \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void *                                     \
name##_ZIP_ITER(struct name *head, name##_cb cb, void *context) {       \
    return __ZIP_ITER(offsetof(struct type, field), (zip_iter_cb)cb,    \
                      context, ZIP_ROOT(head));                         \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void *                                     \
name##_ZIP_ITER_KEY(struct name *head, const keytype *key,              \
                    name##_cb cb, void *context) {                      \
    return __ZIP_ITER_KEY((zip_cmp_cb)cmp, offsetof(struct type, field), \
                          offsetof(struct type, keyfield), key,         \
                          (zip_iter_cb)cb, context, ZIP_ROOT(head));    \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE struct type *                              \
name##_ZIP_ZIP(struct type *left, struct type *right) {                 \
    return (struct type*)                                               \
        __ZIP_ZIP(offsetof(struct type, field), left, right);           \
}                                                                       \
                                                                        \
ZIP_UNUSED static ZIP_INLINE void                                       \
name##_ZIP_UNZIP(struct name *head, const keytype *key,                 \
                 struct name *left, struct name *right) {               \
    __ZIP_UNZIP((zip_cmp_cb)cmp, offsetof(struct type, field),          \
                offsetof(struct type, keyfield), key,                   \
                head, left, right);                                     \
}

/* Internal definitions. Don't use directly. */

void
__ZIP_INSERT(void *h, zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *elm);

void *
__ZIP_REMOVE(void *h, zip_cmp_cb cmp, unsigned short fieldoffset,
             unsigned short keyoffset, void *elm);

void *
__ZIP_ITER(unsigned short fieldoffset, zip_iter_cb cb,
           void *context, void *elm);

void *
__ZIP_ITER_KEY(zip_cmp_cb cmp, unsigned short fieldoffset,
               unsigned short keyoffset, const void *key,
               zip_iter_cb cb, void *context, void *elm);

void *
__ZIP_ZIP(unsigned short fieldoffset, void *left, void *right);

void
__ZIP_UNZIP(zip_cmp_cb cmp, unsigned short fieldoffset,
            unsigned short keyoffset, const void *key,
            void *h, void *l, void *r);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIPTREE_H_ */
