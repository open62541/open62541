/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Julius Pfrommer
 */

#ifndef	ZIPTREE_H_
#define	ZIPTREE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Reusable zip tree implementation. The style is inspired by the BSD
 * sys/queue.h linked list definition.
 *
 * Zip trees were developed in: Tarjan, R. E., Levy, C. C., and Timmel, S. "Zip
 * Trees." arXiv preprint arXiv:1806.06726 (2018).
 *
 * The ZIP_ENTRY definitions are to be contained in the tree entries themselves.
 * Use ZIP_PROTTYPE to define the signature of the zip tree and ZIP_IMPL (in a
 * .c compilation unit) for the method implementations.
 *
 * Zip trees are a probabilistic data structure. Entries are assigned a
 * (nonzero) rank k with probability 1/2^{k+1}. This header file does not assume
 * a specific random number generator. So the rank must be given when an entry
 * is inserted. A fast way (with a single call to a pseudo random generator) to
 * compute the rank is with ZIP_FFS32(random()). The ZIP_FFS32 returns the least
 * significant nonzero bit of a 32bit number. */

#define ZIP_HEAD(name, type)                    \
struct name {                                   \
    struct type *zip_root;                      \
}

#define ZIP_INIT(head) do { (head)->zip_root = NULL; } while (0)
#define ZIP_ROOT(head) (head)->zip_root
#define ZIP_EMPTY(head) (ZIP_ROOT(head) == NULL)

#define ZIP_ENTRY(type)                         \
struct {                                        \
    struct type *zip_left;                      \
    struct type *zip_right;                     \
    unsigned char rank;                         \
}

#define ZIP_LEFT(elm, field) (elm)->field.zip_left
#define ZIP_RIGHT(elm, field) (elm)->field.zip_right
#define ZIP_RANK(elm, field) (elm)->field.rank

/* Shortcuts */
#define ZIP_INSERT(name, head, elm, rank) name##_ZIP_INSERT(head, elm, rank)
#define ZIP_REMOVE(name, head, elm) name##_ZIP_REMOVE(head, elm)
#define ZIP_FIND(name, head, key) name##_ZIP_FIND(head, key)
#define ZIP_MIN(name, head) name##_ZIP_MIN(head)
#define ZIP_MAX(name, head) name##_ZIP_MAX(head)
#define ZIP_ITER(name, head, cb, d) name##_ZIP_ITER(head, cb, d)

/* Zip tree method prototypes */
#define ZIP_PROTTYPE(name, type, keytype)                               \
void name##_ZIP_INSERT(struct name *head, struct type *elm, unsigned char rank); \
void name##_ZIP_REMOVE(struct name *head, struct type *elm);            \
struct type *name##_ZIP_FIND(struct name *head, const keytype *key);    \
struct type *name##_ZIP_MIN(struct name *head);                         \
struct type *name##_ZIP_MAX(struct name *head);                         \
typedef void (*name##_cb)(struct type *elm, void *data);                \
void name##_ZIP_ITER(struct name *head, name##_cb cb, void *data);      \

/* The comparison method "cmp" defined for every zip tree has the signature
 *
 *   enum ZIP_CMP cmpDateTime(const keytype *a, const keytype *b);
 *
 * The entries need an absolute ordering. So ZIP_CMP_EQ must only be returned if
 * a and b point to the same memory. (E.g. assured by unique identifiers.) */
enum ZIP_CMP {
    ZIP_CMP_LESS = -1,
    ZIP_CMP_EQ = 0,
    ZIP_CMP_MORE = 1
};

/* Find the position of the first bit in an unsigned 32bit integer */
#ifdef _MSC_VER
static __inline
#else
static inline
#endif
unsigned char
ZIP_FFS32(unsigned int v) {
    unsigned int t = 1;
    unsigned char r = 1;
    if(v == 0) return 0;
    while((v & t) == 0) {
        t = t << 1; r++;
    }
    return r;
}

/* Zip tree method implementations */
#define ZIP_IMPL(name, type, field, keytype, keyfield, cmp)             \
static struct type *                                                    \
__##name##_ZIP_INSERT(struct type *root, struct type *elm) {            \
    if(!root) {                                                         \
        ZIP_LEFT(elm, field) = NULL;                                    \
        ZIP_RIGHT(elm, field) = NULL;                                   \
        return elm;                                                     \
    }                                                                   \
    if((cmp)(&(elm)->keyfield, &(root)->keyfield) == ZIP_CMP_LESS) {    \
        if(__##name##_ZIP_INSERT(ZIP_LEFT(root, field), elm) == elm) {  \
            if(ZIP_RANK(elm, field) < ZIP_RANK(root, field)) {          \
                ZIP_LEFT(root, field) = elm;                            \
            } else {                                                    \
                ZIP_LEFT(root, field) = ZIP_RIGHT(elm, field);          \
                ZIP_RIGHT(elm, field) = root;                           \
                return elm;                                             \
            }                                                           \
        }                                                               \
    } else {                                                            \
        if(__##name##_ZIP_INSERT(ZIP_RIGHT(root, field), elm) == elm) { \
            if(ZIP_RANK(elm, field) <= ZIP_RANK(root, field)) {         \
                ZIP_RIGHT(root, field) = elm;                           \
            } else {                                                    \
                ZIP_RIGHT(root, field) = ZIP_LEFT(elm, field);          \
                ZIP_LEFT(elm, field) = root;                            \
                return elm;                                             \
            }                                                           \
        }                                                               \
    }                                                                   \
    return root;                                                        \
}                                                                       \
                                                                        \
void                                                                    \
name##_ZIP_INSERT(struct name *head, struct type *elm,                  \
                  unsigned char rank) {                                 \
    ZIP_RANK(elm, field) = rank;                                        \
    ZIP_ROOT(head) = __##name##_ZIP_INSERT(ZIP_ROOT(head), elm);        \
}                                                                       \
                                                                        \
static struct type *                                                    \
__##name##ZIP(struct type *x, struct type *y) {                         \
    if(!x) return y;                                                    \
    if(!y) return x;                                                    \
    if(ZIP_RANK(x, field) < ZIP_RANK(y, field)) {                       \
        ZIP_LEFT(y, field) = __##name##ZIP(x, ZIP_LEFT(y, field));      \
        return y;                                                       \
    }                                                                   \
    ZIP_RIGHT(x, field) = __##name##ZIP(ZIP_RIGHT(x, field), y);        \
    return x;                                                           \
}                                                                       \
                                                                        \
static struct type *                                                    \
__##name##_ZIP_REMOVE(struct type *root, struct type *elm) {            \
    if(root == elm)                                                     \
        return __##name##ZIP(ZIP_LEFT(root, field),                     \
                             ZIP_RIGHT(root, field));                   \
    enum ZIP_CMP eq = (cmp)(&(elm)->keyfield, &(root)->keyfield);       \
    if(eq == ZIP_CMP_LESS) {                                            \
        struct type *left = ZIP_LEFT(root, field);                      \
        if(elm == left)                                                 \
            ZIP_LEFT(root, field) =                                     \
                __##name##ZIP(ZIP_LEFT(left, field),                    \
                              ZIP_RIGHT(left, field));                  \
        else                                                            \
            __##name##_ZIP_REMOVE(left, elm);                           \
    } else {                                                            \
        struct type *right = ZIP_RIGHT(root, field);                    \
        if(elm == right)                                                \
            ZIP_RIGHT(root, field) =                                    \
                __##name##ZIP(ZIP_LEFT(right, field),                   \
                              ZIP_RIGHT(right, field));                 \
        else                                                            \
            __##name##_ZIP_REMOVE(right, elm);                          \
    }                                                                   \
    return root;                                                        \
}                                                                       \
                                                                        \
void                                                                    \
name##_ZIP_REMOVE(struct name *head, struct type *elm) {                \
    ZIP_ROOT(head) = __##name##_ZIP_REMOVE(ZIP_ROOT(head), elm);        \
}                                                                       \
                                                                        \
static struct type *                                                    \
__##name##_ZIP_FIND(struct type *root, const keytype *key) {            \
    if(!root)                                                           \
        return NULL;                                                    \
    enum ZIP_CMP eq = (cmp)(key, &(root)->keyfield);                    \
    if(eq == ZIP_CMP_EQ) {                                              \
        return root;                                                    \
    }                                                                   \
    if(eq == ZIP_CMP_LESS) {                                            \
        return __##name##_ZIP_FIND(ZIP_LEFT(root, field), key);         \
    }                                                                   \
    return __##name##_ZIP_FIND(ZIP_RIGHT(root, field), key);            \
}                                                                       \
                                                                        \
struct type *                                                           \
name##_ZIP_FIND(struct name *head, const keytype *key) {                \
    return __##name##_ZIP_FIND(ZIP_ROOT(head), key);                    \
}                                                                       \
                                                                        \
struct type *                                                           \
name##_ZIP_MIN(struct name *head) {                                     \
    struct type *cur = ZIP_ROOT(head);                                  \
    if(!cur) return NULL;                                               \
    while(ZIP_LEFT(cur, field)) {                                       \
        cur = ZIP_LEFT(cur, field);                                     \
    }                                                                   \
    return cur;                                                         \
}                                                                       \
                                                                        \
struct type *                                                           \
name##_ZIP_MAX(struct name *head) {                                     \
    struct type *cur = ZIP_ROOT(head);                                  \
    if(!cur) return NULL;                                               \
    while(ZIP_RIGHT(cur, field)) {                                      \
        cur = ZIP_RIGHT(cur, field);                                    \
    }                                                                   \
    return cur;                                                         \
}                                                                       \
                                                                        \
static void                                                             \
__##name##_ZIP_ITER(struct type *elm, name##_cb cb, void *data) {       \
    if(!elm)                                                            \
        return;                                                         \
    __##name##_ZIP_ITER(ZIP_LEFT(elm, field), cb, data);                \
    __##name##_ZIP_ITER(ZIP_RIGHT(elm, field), cb, data);               \
    cb(elm, data);                                                      \
}                                                                       \
                                                                        \
void                                                                    \
name##_ZIP_ITER(struct name *head, name##_cb cb, void *data) {          \
    __##name##_ZIP_ITER(ZIP_ROOT(head), cb, data);                      \
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIPTREE_H_ */
