// MIT License
//
// Copyright (c) 2020 Sepehr Taghdisian
// Copyright (c) 2022, 2024 Julius Pfrommer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Very minimal single header JSON5 parser in C99, dervied from jsmn This is the
// modified version of jsmn library Thus main parts of the code is taken from
// jsmn project (https://github.com/zserge/jsmn).
//
// Compliance with JSON5:
//  [x] Object keys may be an ECMAScript 5.1 IdentifierName.
//  [x] Objects may have a single trailing comma.
//  [x] Strings may be single quoted.
//  [x] Strings may span multiple lines by escaping new line characters.
//  [x] Strings may include character escapes.
//  [x] Numbers may be hexadecimal.
//  [x] Numbers may have a leading or trailing decimal point.
//  [x] Numbers may be IEEE 754 positive infinity, negative infinity, and NaN.
//  [x] Numbers may begin with an explicit plus sign.
//  [x] Single and multi-line comments are allowed.
//  [x] Additional white space characters are allowed.
//
// Extensions to JSON5 (more permissive):
//  [x] Root objects can be an array or a primitive value
//  [x] The root object may omit the surrounding brackets
//  [x] Hash ('#') comments out until the end of the line.
//  [x] Strings may include unescaped utf8 bytes
//  [x] Booleans and null are case-insensitive
//  [x] Optionally: Stop early when the first encountered JSON element (object,
//      array, value) has been successfully parsed. Do not return an error when
//      the input string was not processed to its full length. This allows the
//      detection of JSON sub-strings as part of an input "lexer".
//
// Usage:
//  The main function to parse json is `cj5_parse`. Like in jsmn, you provide
//  all tokens to be filled as an array and provide the maximum count The result
//  will be return in `cj5_result` struct, and `num_tokens` will represent the
//  actual token count that is parsed. In case of errors, cj_result.error will
//  be set to an error code Here's a quick example of the usage.
//  
//  #include "cj5.h"
//
//  cj5_token tokens[32];
//  cj5_result r = cj5_parse(g_json, (int)strlen(g_json), tokens, 32, NULL);
//  if(r.error != CJ5_ERROR_NONE) {
//      if(r.error == CJ5_ERROR_OVERFLOW) {
//          // you can use r.num_tokens to determine the actual token count and reparse
//          printf("Error: line: %d, col: %d\n", r.error_line, r.error_code);    
//      }
//  }

#ifndef __CJ5_H_
#define __CJ5_H_

#ifdef __cplusplus
# define CJ5_API extern "C"
#else
# define CJ5_API
#endif

#if !defined(_MSC_VER) || _MSC_VER >= 1800
# include <stdint.h>
# include <stdbool.h>
#else
# include "ms_stdint.h"
# if !defined(__bool_true_false_are_defined)
#  define bool unsigned char
#  define true 1
#  define false 0
#  define __bool_true_false_are_defined
# endif
#endif

typedef enum cj5_token_type {
    CJ5_TOKEN_OBJECT = 0,
    CJ5_TOKEN_ARRAY,
    CJ5_TOKEN_NUMBER,
    CJ5_TOKEN_STRING,
    CJ5_TOKEN_BOOL,
    CJ5_TOKEN_NULL
} cj5_token_type;

typedef enum cj5_error_code {
    CJ5_ERROR_NONE = 0,
    CJ5_ERROR_INVALID,       // Invalid character/syntax
    CJ5_ERROR_INCOMPLETE,    // Incomplete JSON string
    CJ5_ERROR_OVERFLOW,      // Token buffer overflow (see cj5_result.num_tokens)
    CJ5_ERROR_NOTFOUND
} cj5_error_code;

typedef struct cj5_token {
    cj5_token_type type;
    unsigned int start;     // Start position in the json5 string
    unsigned int end;       // Position of the last character (included)
    unsigned int size;      // For objects and arrays the number of direct
                            // children. Note that this is *not* the number of
                            // overall (recursively nested) child tokens. For
                            // other tokens the length of token in the json
                            // encoding.
    unsigned int parent_id; // The root object is at position zero. It is an
                            // object that has itself as parent.
} cj5_token;

typedef struct cj5_result {
    cj5_error_code error;
    unsigned int error_line;
    unsigned int error_col;
    unsigned int num_tokens;
    const cj5_token* tokens;
    const char* json5;
} cj5_result;

typedef struct cj5_options {
    bool stop_early; /* Return when the first element was parsed. Otherwise an
                      * error is returned if the input was not fully
                      * processed. (default: false) */
} cj5_options;

/* Options can be NULL */
CJ5_API cj5_result
cj5_parse(const char *json5, unsigned int len,
          cj5_token *tokens, unsigned int max_tokens,
          cj5_options *options);

CJ5_API cj5_error_code
cj5_get_bool(const cj5_result *r, unsigned int tok_index, bool *out);

CJ5_API cj5_error_code
cj5_get_float(const cj5_result *r, unsigned int tok_index, double *out);

CJ5_API cj5_error_code
cj5_get_int(const cj5_result *r, unsigned int tok_index, int64_t *out);

CJ5_API cj5_error_code
cj5_get_uint(const cj5_result *r, unsigned int tok_index, uint64_t *out);

// Replaces escape characters, utf8 codepoints, etc.
// The buffer shall have a length of at least token->size + 1.
// Upon success, the length is written to buflen.
// The output string is terminated with \0.
CJ5_API cj5_error_code
cj5_get_str(const cj5_result *r, unsigned int tok_index,
            char *buf, unsigned int *buflen);

// Skips the (nested) structure that starts at the current index. The index is
// updated accordingly. Afterwards it points to the beginning of the following
// structure.
//
// Attention! The index can point to the first element after the token array if
// the root object is skipped.
//
// Cannot fail as long as the token array is the result of cj5_parse.
CJ5_API void
cj5_skip(const cj5_result *r, unsigned int *tok_index);

// Lookup of a key within an object (linear search).
// The current token (index) must point to an object.
// The error code CJ5_ERROR_NOTFOUND is returned if the key is not present.
// Otherwise the index is updated to point to the value associated with the key.
CJ5_API cj5_error_code
cj5_find(const cj5_result *r, unsigned int *tok_index, const char *key);

#endif /* __CJ5_H_ */
