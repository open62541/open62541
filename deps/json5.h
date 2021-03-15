/*
    JSON5 parser module

    License:
    This software is dual-licensed to the public domain and under the following
    license: you are granted a perpetual, irrevocable license to copy, modify,
    publish, and distribute this file as you see fit.
    No warranty is implied, use at your own risk.

    Credits:
    Dominik Madarasz (GitHub: zaklaus)
    r-lyeh (fork)

    Version History:
    2.1.0 - negative numbers fix, comment parsing fix, invalid memaccess fix (@r-lyeh)
    2.0.9 - zpl 4.0.0 support
    2.0.8 - Small cleanup in README and test file
    2.0.7 - Small fixes for tiny cpp warnings
    2.0.5 - Fix for bad access on deallocation
    2.0.4 - Small fix for cpp issues
    2.0.3 - Small bugfix in name with underscores
    2.0.1 - Catch error in name
    2.0.0 - Added basic error handling
    1.4.0 - Added Infinity and NaN constants
    1.3.0 - Added multi-line backtick strings
    1.2.0 - More JSON5 features and bugfixes
    1.1.1 - Small mistake fixed
    1.1.0 - Basic JSON5 support, comments and fixes
    1.0.4 - Header file fixes
    1.0.0 - Initial version
*/

/*

@todo change api to:

json_value * json_parse (const json_char * json, size_t length, char *error); // error = buffer to store error, if any.
void json_value_free(json_value *);

The type field of json_value is one of:

json_array (see ->len, ->values[x])
json_object (see ->len, ->values[x], ->names[x])
json_integer (see ->num)
json_boolean (see ->num)
json_double (see ->dbl)
json_string (see ->len, ->str)
json_null ()

*/

#ifndef JSON5_H_
#define JSON5_H_

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum json5_type {
    json5_type_object = 0,
    json5_type_string,
    json5_type_multistring,
    json5_type_array,
    json5_type_integer,
    json5_type_real,
    json5_type_null,
    json5_type_false,
    json5_type_true,
} json5_type;

typedef struct json5_object {
    char *name;
    union {
        struct json5_object* elements;
        struct json5_object* nodes;
        int64_t integer;
        double real;
        char *string;
    } content;
    uint32_t size;
    json5_type type;
} json5_object;

typedef enum json5_error {
    json5_error_none = 0,
    json5_error_invalid_name,
    json5_error_invalid_value,
    json5_error_no_memory,
    json5_error_internal
} json5_error;

json5_error json5_parse(json5_object *root, char *source);
void json5_clear(json5_object *root);

#ifdef __cplusplus
}
#endif

#endif /* JSON5_H_ */
