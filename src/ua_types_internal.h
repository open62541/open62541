#ifndef UA_TYPES_INTERNAL_H_
#define UA_TYPES_INTERNAL_H_

/* Macros for type implementations */

#define UA_TYPE_DEFAULT(TYPE)            \
    UA_TYPE_DELETE_DEFAULT(TYPE)         \
    UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    UA_TYPE_INIT_DEFAULT(TYPE)           \
    UA_TYPE_NEW_DEFAULT(TYPE)            \
    UA_TYPE_COPY_DEFAULT(TYPE)           \
    
#define UA_TYPE_NEW_DEFAULT(TYPE)                             \
    TYPE * TYPE##_new() {                                     \
        TYPE *p = UA_alloc(sizeof(TYPE));                     \
        if(p) TYPE##_init(p);                                 \
        return p;                                             \
    }

#define UA_TYPE_INIT_DEFAULT(TYPE) \
    void TYPE##_init(TYPE * p) {   \
        *p = (TYPE)0;              \
    }

#define UA_TYPE_INIT_AS(TYPE, TYPE_AS) \
    void TYPE##_init(TYPE * p) {       \
        TYPE_AS##_init((TYPE_AS *)p);  \
    }

#define UA_TYPE_DELETE_DEFAULT(TYPE) \
    void TYPE##_delete(TYPE *p) {    \
        TYPE##_deleteMembers(p);     \
        UA_free(p);                  \
    }

#define UA_TYPE_DELETE_AS(TYPE, TYPE_AS) \
    void TYPE##_delete(TYPE * p) {       \
        TYPE_AS##_delete((TYPE_AS *)p);  \
    }

#define UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    void TYPE##_deleteMembers(TYPE * p) { return; }

#define UA_TYPE_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
    void TYPE##_deleteMembers(TYPE * p) { TYPE_AS##_deleteMembers((TYPE_AS *)p); }

/* Use only when the type has no arrays. Otherwise, implement deep copy */
#define UA_TYPE_COPY_DEFAULT(TYPE)                             \
    UA_StatusCode TYPE##_copy(TYPE const *src, TYPE *dst) {    \
        *dst = *src;                                           \
        return UA_STATUSCODE_GOOD;                             \
    }

#define UA_TYPE_COPY_AS(TYPE, TYPE_AS)                         \
    UA_StatusCode TYPE##_copy(TYPE const *src, TYPE *dst) {    \
        return TYPE_AS##_copy((TYPE_AS *)src, (TYPE_AS *)dst); \
    }

#ifdef DEBUG //print functions only in debug mode
#define UA_TYPE_PRINT_AS(TYPE, TYPE_AS)              \
    void TYPE##_print(TYPE const *p, FILE *stream) { \
        TYPE_AS##_print((TYPE_AS *)p, stream);       \
    }
#else
#define UA_TYPE_PRINT_AS(TYPE, TYPE_AS)
#endif

#define UA_TYPE_AS(TYPE, TYPE_AS)           \
    UA_TYPE_NEW_DEFAULT(TYPE)               \
    UA_TYPE_INIT_AS(TYPE, TYPE_AS)          \
    UA_TYPE_DELETE_AS(TYPE, TYPE_AS)        \
    UA_TYPE_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
    UA_TYPE_COPY_AS(TYPE, TYPE_AS)          \
    UA_TYPE_PRINT_AS(TYPE, TYPE_AS)

#endif /* UA_TYPES_INTERNAL_H_ */
