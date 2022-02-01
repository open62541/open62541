// cj5.h: Very minimal single header JSON5 parser in C99, dervied from jsmn
//        This is the modified version of jsmn library
//        Thus main parts of the code is taken from jsmn project (https://github.com/zserge/jsmn)
//
// MIT License
//
// Copyright (c) 2020 Sepehr Taghdisian
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
//  [x] Multiline comments
//
// Usage:
//  The main function to parse json is `cj5_parse`. 
//  like jsmn, you provide all tokens to be filled as an array and provide the maximum count
//  The result will be return in `cj5_result` struct, and `num_tokens` will represent the actual
//  token count that is parsed. 
//  In case of errors, cj_result.error will be set to an error code
//  Here's a quick example of the usage, first define CJ5_IMPLEMENT to include the implementation:
//  
//        #define CJ5_IMPLEMENT
//        #include "cj5.h"
//
//        cj5_token tokens[32];
//        cj5_result r = cj5_parse(g_json, (int)strlen(g_json), tokens, 32);
//        
//        if (r.error != CJ5_ERROR_NONE) {
//          if (r.error == CJ5_ERROR_OVERFLOW) {
//              // you can use r.num_tokens to determine the actual token count and reparse
//              printf("Error: line: %d, col: %d\n", r.error_line, r.error_code);    
//          }
//        } else {
//          // use token helper functions (see below) to access the values 
//          float my_num = cj5_seekget_float(&r, 0, "my_num", 0 /* default value if not found*/ );
//        } 
// 
// Customization:
//  This library doesn't use any memory allocations so hooray for that !
//  But in order to not use default stdc functions, you can override those by defining these macros:
//      - CJ5_IMPLEMENT: include implementation for cj5
//      - CJ5_ASSERT(e): replace stdc `assert` macro with your own version
//      - CJ5_MEMSET(dst, value, size): replace 'memset' function with your own
//      - CJ5_MEMCPY(dst, src, size): replace 'memcpy' function with your own
//      - CJ5_TOKEN_HELPERS: add token helper functions (default=ON), you can skip these by definining:
//                           #define CJ5_TOKEN_HELPERS 0, before including the header
//      - CJ5_API: API decleration can be override by defining this macro. (default is extern)
//                 example: #define CJ5_API static
// 
#pragma once

#include <stdbool.h>    // bool
#include <stdint.h>     // uint32_t, int64_t, etc.

#ifndef CJ5_TOKEN_HELPERS
#    define CJ5_TOKEN_HELPERS 1
#endif

#ifndef CJ5_API
#    ifdef __cplusplus
#        define CJ5_API extern "C"
#    else
#        define CJ5_API
#    endif
#endif

typedef enum cj5_token_type {
    CJ5_TOKEN_OBJECT = 0,
    CJ5_TOKEN_ARRAY,
    CJ5_TOKEN_NUMBER,
    CJ5_TOKEN_STRING,
    CJ5_TOKEN_BOOL,
    CJ5_TOKEN_NULL
} cj5_token_type;

// sub-type for NUMBER token type
// this is merely a hint for the user to see what type of number the token contains
typedef enum cj5_token_number_type {
    CJ5_TOKEN_NUMBER_UNKNOWN = 0,
    CJ5_TOKEN_NUMBER_FLOAT,
    CJ5_TOKEN_NUMBER_INT,
    CJ5_TOKEN_NUMBER_HEX
} cj5_token_number_type;

typedef enum cj5_error_code {
    CJ5_ERROR_NONE = 0,
    CJ5_ERROR_INVALID,       // invalid character/syntax
    CJ5_ERROR_INCOMPLETE,    // incomplete json string
    CJ5_ERROR_OVERFLOW       // token buffer overflow, need more tokens (see cj5_result.num_tokens)
} cj5_error_code;

typedef struct cj5_token {
    cj5_token_type type;
    union {
        cj5_token_number_type num_type;
        uint32_t key_hash;
    };
    int start;
    int end;
    int size;
    int parent_id;      // = -1 if there is no parent
} cj5_token;

typedef struct cj5_result {
    cj5_error_code error;
    int error_line;
    int error_col;
    int num_tokens;
    const cj5_token* tokens;
    const char* json5;
} cj5_result;

CJ5_API cj5_result cj5_parse(const char* json5, int len, cj5_token* tokens, int max_tokens);

// token helpers
#if CJ5_TOKEN_HELPERS
CJ5_API int cj5_seek(cj5_result* r, int parent_id, const char* key);
CJ5_API int cj5_seek_hash(cj5_result* r, int parent_id, const uint32_t key_hash);
CJ5_API int cj5_seek_recursive(cj5_result* r, int parent_id, const char* key);
CJ5_API const char* cj5_get_string(cj5_result* r, int id, char* str, int max_str);
CJ5_API double cj5_get_double(cj5_result* r, int id);
CJ5_API float cj5_get_float(cj5_result* r, int id);
CJ5_API int cj5_get_int(cj5_result* r, int id);
CJ5_API uint32_t cj5_get_uint(cj5_result* r, int id);
CJ5_API uint64_t cj5_get_uint64(cj5_result* r, int id);
CJ5_API int64_t cj5_get_int64(cj5_result* r, int id);
CJ5_API bool cj5_get_bool(cj5_result* r, int id);
CJ5_API double cj5_seekget_double(cj5_result* r, int parent_id, const char* key, double def_val);
CJ5_API float cj5_seekget_float(cj5_result* r, int parent_id, const char* key, float def_val);
CJ5_API int cj5_seekget_array_int16(cj5_result* r, int parent_id, const char* key, int16_t* values, int max_values);
CJ5_API int cj5_seekget_array_uint16(cj5_result* r, int parent_id, const char* key, uint16_t* values, int max_values);
CJ5_API int cj5_seekget_int(cj5_result* r, int parent_id, const char* key, int def_val);
CJ5_API uint32_t cj5_seekget_uint(cj5_result* r, int parent_id, const char* key, uint32_t def_val);
CJ5_API uint64_t cj5_seekget_uint64(cj5_result* r, int parent_id, const char* key, uint64_t def_val);
CJ5_API int64_t cj5_seekget_int64(cj5_result* r, int parent_id, const char* key, int64_t def_val);
CJ5_API bool cj5_seekget_bool(cj5_result* r, int parent_id, const char* key, bool def_val);
CJ5_API const char* cj5_seekget_string(cj5_result* r, int parent_id, const char* key, char* str, int max_str, const char* def_val);
CJ5_API int cj5_seekget_array_double(cj5_result* r, int parent_id, const char* key, double* values, int max_values);
CJ5_API int cj5_seekget_array_float(cj5_result* r, int parent_id, const char* key, float* values, int max_values);
CJ5_API int cj5_seekget_array_int(cj5_result* r, int parent_id, const char* key, int* values, int max_values);
CJ5_API int cj5_seekget_array_uint(cj5_result* r, int parent_id, const char* key, uint32_t* values, int max_values);
CJ5_API int cj5_seekget_array_uint64(cj5_result* r, int parent_id, const char* key, uint64_t* values, int max_values);
CJ5_API int cj5_seekget_array_int64(cj5_result* r, int parent_id, const char* key, int64_t* values, int max_values);
CJ5_API int cj5_seekget_array_bool(cj5_result* r, int parent_id, const char* key, bool* values, int max_values);
CJ5_API int cj5_seekget_array_string(cj5_result* r, int parent_id, const char* key, char** strs, int max_str, int max_values);
CJ5_API int cj5_get_array_elem(cj5_result* r, int id, int index);
CJ5_API int cj5_get_array_elem_incremental(cj5_result* r, int id, int index, int prev_elem);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
#if defined(CJ5_IMPLEMENT)

// optional: override asset
#    ifndef CJ5_ASSERT
#        include <assert.h>
#        define CJ5_ASSERT(_e) assert(_e)
#    endif

// optional: override memcpy
#    ifndef CJ5_MEMCPY
#        include <string.h>    // memcpy
#        define CJ5_MEMCPY(_dst, _src, _n) memcpy((_dst), (_src), (_n))
#    endif

#    ifndef CJ5_MEMSET
#        include <string.h>
#        define CJ5_MEMSET(_dst, _val, _size) memset((_dst), (_val), (_size))
#    endif

#    define CJ5__ARCH_64BIT 0
#    define CJ5__ARCH_32BIT 0
#    if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__64BIT__) || \
        defined(__mips64) || defined(__powerpc64__) || defined(__ppc64__) || defined(__LP64__)
#        undef CJ5__ARCH_64BIT
#        define CJ5__ARCH_64BIT 64
#    else
#        undef CJ5__ARCH_32BIT
#        define CJ5__ARCH_32BIT 32
#    endif    //

#    if defined(_MSC_VER)
#        define CJ5__RESTRICT __restrict
#    else
#        define CJ5__RESTRICT __restrict__
#    endif

#    define CJ5__UNUSED(_a) (void)(_a)

#    define CJ5__FOURCC(_a, _b, _c, _d) \
        (((uint32_t)(_a) | ((uint32_t)(_b) << 8) | ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

static const uint32_t CJ5__NULL_FOURCC = CJ5__FOURCC('n', 'u', 'l', 'l');
static const uint32_t CJ5__TRUE_FOURCC = CJ5__FOURCC('t', 'r', 'u', 'e');
static const uint32_t CJ5__FALSE_FOURCC = CJ5__FOURCC('f', 'a', 'l', 's');

static const uint32_t CJ5__FNV1_32_INIT = 0x811c9dc5;
static const uint32_t CJ5__FNV1_32_PRIME = 0x01000193;

typedef struct cj5__parser {
    int pos;
    int next_id;
    int super_id;
    int line;
} cj5__parser;

static inline uint32_t cj5__hash_fnv32(const char* start, const char* end)
{
    const char* bp = start;
    const char* be = end;    // start + len

    uint32_t hval = CJ5__FNV1_32_INIT;
    while (bp < be) {
        hval ^= (uint32_t)*bp++;
        hval *= CJ5__FNV1_32_PRIME;
    }

    return hval;
}

static inline bool cj5__isspace(char ch)
{
    return (uint32_t)(ch - 1) < 32 && ((0x80001F00 >> (uint32_t)(ch - 1)) & 1) == 1;
}

static inline bool cj5__isrange(char ch, char from, char to)
{
    return (uint8_t)(ch - from) <= (uint8_t)(to - from);
}

static inline bool cj5__isupperchar(char ch)
{
    return cj5__isrange(ch, 'A', 'Z');
}

static inline bool cj5__islowerchar(char ch)
{
    return cj5__isrange(ch, 'a', 'z');
}

static inline bool cj5__isnum(char ch)
{
    return cj5__isrange(ch, '0', '9');
}

static inline char* cj5__strcpy(char* CJ5__RESTRICT dst, int dst_sz, const char* CJ5__RESTRICT src,
                                int num)
{
    const int _max = dst_sz - 1;
    const int _num = (num < _max ? num : _max);
    if (_num > 0) {
        CJ5_MEMCPY(dst, src, _num);
    }
    dst[_num] = '\0';
    return dst;
}

// https://github.com/lattera/glibc/blob/master/string/strlen.c
static int cj5__strlen(const char* str)
{
    const char* char_ptr;
    const uintptr_t* longword_ptr;
    uintptr_t longword, himagic, lomagic;

    for (char_ptr = str; ((uintptr_t)char_ptr & (sizeof(longword) - 1)) != 0; ++char_ptr) {
        if (*char_ptr == '\0')
            return (int)(intptr_t)(char_ptr - str);
    }
    longword_ptr = (uintptr_t*)char_ptr;
    himagic = 0x80808080L;
    lomagic = 0x01010101L;
#    if CJ5__ARCH_64BIT
    /* 64-bit version of the magic.  */
    /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
    himagic = ((himagic << 16) << 16) | himagic;
    lomagic = ((lomagic << 16) << 16) | lomagic;
#    endif

    for (;;) {
        longword = *longword_ptr++;

        if (((longword - lomagic) & ~longword & himagic) != 0) {
            const char* cp = (const char*)(longword_ptr - 1);

            if (cp[0] == 0)
                return (int)(intptr_t)(cp - str);
            if (cp[1] == 0)
                return (int)(intptr_t)(cp - str + 1);
            if (cp[2] == 0)
                return (int)(intptr_t)(cp - str + 2);
            if (cp[3] == 0)
                return (int)(intptr_t)(cp - str + 3);
#    if CJ5__ARCH_64BIT
            if (cp[4] == 0)
                return (int)(intptr_t)(cp - str + 4);
            if (cp[5] == 0)
                return (int)(intptr_t)(cp - str + 5);
            if (cp[6] == 0)
                return (int)(intptr_t)(cp - str + 6);
            if (cp[7] == 0)
                return (int)(intptr_t)(cp - str + 7);
#    endif
        }
    }

    #ifndef _MSC_VER
    CJ5_ASSERT(0);
    return -1;
    #endif
}

static inline cj5_token* cj5__alloc_token(cj5__parser* parser, cj5_token* tokens, int max_tokens)
{
    if (!tokens || parser->next_id >= max_tokens) {
        return NULL;
    }

    cj5_token* token = &tokens[parser->next_id++];
    CJ5_MEMSET(token, 0x0, sizeof(cj5_token));
    token->start = -1;
    token->end = -1;
    token->parent_id = -1;
    return token;
}

static inline void cj5__set_error(cj5_result* r, cj5_error_code code, int line, int col)
{
    r->error = code;
    r->error_line = line + 1;
    r->error_col = col + 1;
}

static bool cj5__parse_primitive(cj5__parser* parser, cj5_result* r, const char* json5, int len,
                                 cj5_token* tokens, int max_tokens)
{
    cj5_token* token;
    int start = parser->pos;
    int line_start = start;
    bool keyname = false;
    bool new_line = false;

    for (; parser->pos < len; parser->pos++) {
        switch (json5[parser->pos]) {
        case '\n':
            line_start = parser->pos;
            new_line = true;
            goto found;
        case ':':
            keyname = true;
            goto found;
        case '\t':
        case '\r':
        case ' ':
        case ',':
        case ']':
        case '}':
            goto found;
        default:
            break;
        }

        if (json5[parser->pos] < 32 || json5[parser->pos] >= 127) {
            cj5__set_error(r, CJ5_ERROR_INVALID, parser->line, parser->pos - line_start);
            parser->pos = start;
            return false;
        }
    }

    cj5__set_error(r, CJ5_ERROR_INCOMPLETE, parser->line, parser->pos - line_start);
    parser->pos = start;
    return false;

found:
    token = cj5__alloc_token(parser, tokens, max_tokens);
    if (token == NULL) {
        r->error = CJ5_ERROR_OVERFLOW;
        --parser->pos;
        return true;
    }

    cj5_token_type type;
    cj5_token_number_type num_type = CJ5_TOKEN_NUMBER_UNKNOWN;
    if (keyname) {
        // JSON5: it is likely a key-name, validate and interpret as string
        for (int i = start; i < parser->pos; i++) {
            if (cj5__islowerchar(json5[i]) || cj5__isupperchar(json5[i]) || json5[i] == '_') {
                continue;
            }

            if (cj5__isnum(json5[i])) {
                if (i == start) {
                    cj5__set_error(r, CJ5_ERROR_INVALID, parser->line, parser->pos - line_start);
                    parser->pos = start;
                    return false;
                }
                continue;
            }

            cj5__set_error(r, CJ5_ERROR_INVALID, parser->line, parser->pos - line_start);
            parser->pos = start;
            return false;
        }

        type = CJ5_TOKEN_STRING;
    } else {
        // detect other types, subtypes
        // note that we have to use memcpy here or we will get unaligned access on some
        uint32_t fourcc_;
        CJ5_MEMCPY(&fourcc_, &json5[start], 4);
        uint32_t* fourcc = &fourcc_;
 
        if (*fourcc == CJ5__NULL_FOURCC) {
            type = CJ5_TOKEN_NULL;
        } else if (*fourcc == CJ5__TRUE_FOURCC) {
            type = CJ5_TOKEN_BOOL;
        } else if (*fourcc == CJ5__FALSE_FOURCC) {
            type = CJ5_TOKEN_BOOL;
        } else {
            num_type = CJ5_TOKEN_NUMBER_INT;
            // hex number
            if (json5[start] == '0' && start < parser->pos + 1 && json5[start + 1] == 'x') {
                start = start + 2;
                for (int i = start; i < parser->pos; i++) {
                    if (!(cj5__isrange(json5[i], '0', '9') || cj5__isrange(json5[i], 'A', 'F') ||
                          cj5__isrange(json5[i], 'a', 'f'))) {
                        cj5__set_error(r, CJ5_ERROR_INVALID, parser->line,
                                       parser->pos - line_start);
                        parser->pos = start;
                        return false;
                    }
                }
                num_type = CJ5_TOKEN_NUMBER_HEX;
            } else {
                int start_index = start;
                if (json5[start] == '+') {
                    ++start_index;
                    ++start;
                } else if (json5[start] == '-') {
                    ++start_index;
                }

                for (int i = start_index; i < parser->pos; i++) {
                    if (json5[i] == '.') {
                        if (num_type == CJ5_TOKEN_NUMBER_FLOAT) {
                            cj5__set_error(r, CJ5_ERROR_INVALID, parser->line,
                                           parser->pos - line_start);
                            parser->pos = start;
                            return false;
                        }
                        num_type = CJ5_TOKEN_NUMBER_FLOAT;
                        continue;
                    }

                    if (!cj5__isnum(json5[i])) {
                        cj5__set_error(r, CJ5_ERROR_INVALID, parser->line,
                                       parser->pos - line_start);
                        parser->pos = start;
                        return false;
                    }
                }
            }

            type = CJ5_TOKEN_NUMBER;
        }
    }

    if (new_line) {
        ++parser->line;
    }

    token->type = type;
    if (type == CJ5_TOKEN_STRING) {
        token->key_hash = cj5__hash_fnv32(&json5[start], &json5[parser->pos]);
    } else {
        token->num_type = num_type;
    }
    token->start = start;
    token->end = parser->pos;
    token->parent_id = parser->super_id;
    --parser->pos;
    return true;
}

static bool cj5__parse_string(cj5__parser* parser, cj5_result* r, const char* json5, int len,
                              cj5_token* tokens, int max_tokens)
{
    cj5_token* token;
    int start = parser->pos;
    int line_start = start;
    char str_open = json5[start];
    ++parser->pos;

    for (; parser->pos < len; parser->pos++) {
        char c = json5[parser->pos];

        // end of string
        if (str_open == c) {
            token = cj5__alloc_token(parser, tokens, max_tokens);
            if (token == NULL) {
                r->error = CJ5_ERROR_OVERFLOW;
                return true;
            }

            token->type = CJ5_TOKEN_STRING;
            token->start = start + 1;
            token->end = parser->pos;
            token->parent_id = parser->super_id;

            return true;
        }

        if (c == '\\' && parser->pos + 1 < len) {
            ++parser->pos;
            switch (json5[parser->pos]) {
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            case 'u':
                ++parser->pos;
                for (int i = 0; i < 4 && parser->pos < len; i++) {
                    /* If it isn't a hex character we have an error */
                    if (!((json5[parser->pos] >= 48 && json5[parser->pos] <= 57) ||   /* 0-9 */
                          (json5[parser->pos] >= 65 && json5[parser->pos] <= 70) ||   /* A-F */
                          (json5[parser->pos] >= 97 && json5[parser->pos] <= 102))) { /* a-f */
                        cj5__set_error(r, CJ5_ERROR_INVALID, parser->line,
                                       parser->pos - line_start);
                        parser->pos = start;
                        return false;
                    }
                    parser->pos++;
                }

                --parser->pos;
                break;
            case '\n':
                line_start = parser->pos;
                ++parser->line;
                break;
            default:
                cj5__set_error(r, CJ5_ERROR_INVALID, parser->line, parser->pos - line_start);
                parser->pos = start;
                return false;
            }
        }
    }

    parser->pos = start;
    return true;
}

static void cj5__skip_comment(cj5__parser* parser, const char* json5, int len)
{
    for (; parser->pos < len; parser->pos++) {
        if (json5[parser->pos] == '\n' || json5[parser->pos] == '\r') {
            return;
        }
    }
}

static void cj5__skip_multiline_comment(cj5__parser* parser, const char* json5, int len)
{
    for (; parser->pos < len; parser->pos++) {
        if (json5[parser->pos] == '*' && parser->pos < (len - 1) && json5[parser->pos+1] == '/') {
            return;
        }
    }
}

cj5_result cj5_parse(const char* json5, int len, cj5_token* tokens, int max_tokens)
{
    cj5__parser parser;
    CJ5_MEMSET(&parser, 0x0, sizeof(parser));
    parser.super_id = -1;

    cj5_result r;
    CJ5_MEMSET(&r, 0x0, sizeof(r));

    cj5_token* token;
    int count = parser.next_id;
    bool can_comment = false;

    for (; parser.pos < len; parser.pos++) {
        char c;
        cj5_token_type type;

        c = json5[parser.pos];
        switch (c) {
        case '{':
        case '[':
            can_comment = false;
            count++;
            token = cj5__alloc_token(&parser, tokens, max_tokens);
            if (token == NULL) {
                r.error = CJ5_ERROR_OVERFLOW;
                break;
            }

            if (parser.super_id != -1) {
                cj5_token* super_token = &tokens[parser.super_id];
                token->parent_id = parser.super_id;
                if (++super_token->size == 1 && super_token->type == CJ5_TOKEN_STRING) {
                    super_token->key_hash =
                        cj5__hash_fnv32(&json5[super_token->start], &json5[super_token->end]);
                }
            }

            token->type = (c == '{' ? CJ5_TOKEN_OBJECT : CJ5_TOKEN_ARRAY);
            token->start = parser.pos;
            parser.super_id = parser.next_id - 1;
            break;

        case '}':
        case ']':
            can_comment = false;
            if (!tokens || r.error == CJ5_ERROR_OVERFLOW) {
                break;
            }
            type = (c == '}' ? CJ5_TOKEN_OBJECT : CJ5_TOKEN_ARRAY);

            if (parser.next_id < 1) {
                cj5__set_error(&r, CJ5_ERROR_INVALID, parser.line, parser.pos - parser.line);
                return r;
            }

            token = &tokens[parser.next_id - 1];
            for (;;) {
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        cj5__set_error(&r, CJ5_ERROR_INVALID, parser.line,
                                       parser.pos - parser.line);
                        return r;
                    }
                    token->end = parser.pos + 1;
                    parser.super_id = token->parent_id;
                    break;
                }

                if (token->parent_id == -1) {
                    if (token->type != type || parser.super_id == -1) {
                        cj5__set_error(&r, CJ5_ERROR_INVALID, parser.line,
                                       parser.pos - parser.line);
                        return r;
                    }
                    break;
                }

                token = &tokens[token->parent_id];
            }
            break;

        case '\"':
        case '\'':
            can_comment = false;
            // JSON5: strings can start with \" or \'
            cj5__parse_string(&parser, &r, json5, len, tokens, max_tokens);
            if (r.error && r.error != CJ5_ERROR_OVERFLOW) {
                return r;
            }
            count++;
            if (parser.super_id != -1 && tokens && r.error != CJ5_ERROR_OVERFLOW) {
                if (++tokens[parser.super_id].size == 1 &&
                    tokens[parser.super_id].type == CJ5_TOKEN_STRING) {
                    // it's not a value, it's a key, so hash it
                    tokens[parser.super_id].key_hash = cj5__hash_fnv32(
                        &json5[tokens[parser.super_id].start], &json5[tokens[parser.super_id].end]);
                }
            }
            break;

        case '\r':
            can_comment = true;
            break;
        case '\n':
            ++parser.line;
            can_comment = true;
            break;
        case '\t':
        case ' ':
            break;

        case ':':
            can_comment = false;
            parser.super_id = parser.next_id - 1;
            break;

        case ',':
            can_comment = false;
            if (tokens != NULL && parser.super_id != -1 && r.error != CJ5_ERROR_OVERFLOW &&
                tokens[parser.super_id].type != CJ5_TOKEN_ARRAY &&
                tokens[parser.super_id].type != CJ5_TOKEN_OBJECT) {
                parser.super_id = tokens[parser.super_id].parent_id;
            }
            break;
        case '/':
            if (can_comment && parser.pos < len - 1) {
                if (json5[parser.pos + 1] == '/') {
                    cj5__skip_comment(&parser, json5, len);
                } else if (json5[parser.pos + 1] == '*') {
                    cj5__skip_multiline_comment(&parser, json5, len);
                }
            } 
            break;

        default:
            cj5__parse_primitive(&parser, &r, json5, len, tokens, max_tokens);
            if (r.error && r.error != CJ5_ERROR_OVERFLOW) {
                return r;
            }
            can_comment = false;
            count++;
            if (parser.super_id != -1 && tokens && r.error != CJ5_ERROR_OVERFLOW) {
                if (++tokens[parser.super_id].size == 1 &&
                    tokens[parser.super_id].type == CJ5_TOKEN_STRING) {
                    tokens[parser.super_id].key_hash = cj5__hash_fnv32(
                        &json5[tokens[parser.super_id].start], &json5[tokens[parser.super_id].end]);
                }
            }
            break;
        }
    }

    if (tokens && r.error != CJ5_ERROR_OVERFLOW) {
        for (int i = parser.next_id - 1; i >= 0; i--) {
            // unmatched object or array ?
            if (tokens[i].start != -1 && tokens[i].end == -1) {
                cj5__set_error(&r, CJ5_ERROR_INCOMPLETE, parser.line, parser.pos - parser.line);
                return r;
            }
        }
    }

    r.num_tokens = count;
    r.tokens = tokens;
    r.json5 = json5;
    return r;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions to work with tokens
#    if CJ5_TOKEN_HELPERS
#        include <stdlib.h>

static int cj5__seek_recursive(cj5_result* r, int parent_id, uint32_t key_hash)
{
    const cj5_token* parent_tok = &r->tokens[parent_id];

    for (int i = parent_id + 1, count = 0; i < r->num_tokens && count < parent_tok->size; i++) {
        const cj5_token* tok = &r->tokens[i];

        if (tok->size != 1 || tok->type != CJ5_TOKEN_STRING) {
            continue;
        }

        if (parent_id == tok->parent_id) {
            CJ5_ASSERT((i + 1) < r->num_tokens);
            if (key_hash == tok->key_hash) {
                return i + 1;    // return next "value" token (array/objects and primitive values)
            } else if (r->tokens[i + 1].size) {
                int found_id = cj5__seek_recursive(r, i + 1, key_hash);
                if (found_id != -1) {
                    return found_id;
                }
            }
            count++;
        }
    }

    return -1;
}

static bool cj5__tofloat(const char* str, double* ofloat)
{
    // skip leading whitespace
    while (*str && cj5__isspace(*str)) {
        str++;
    }
    int sign = 1;

    const char* pC = str;
    if (*pC == '-') {
        sign = -1;
        pC++;
    }

    // parse integer part
    int64_t tmp = 0;
    while (*pC >= '0' && *pC <= '9') {
        tmp *= 10;
        tmp += *pC - '0';
        pC++;
    }

    if (*pC == 0) {
        *ofloat = (double)(sign >= 0 ? tmp : -tmp);
        return true;
    }

    // Parse fraction
    if (*pC == '.') {
        pC++;

        int64_t divisor = sign;
        while (*pC >= '0' && *pC <= '9') {
            divisor *= 10;
            tmp *= 10;
            tmp += *pC - '0';
            pC++;
        }

        // Skip tailing whitespace
        while (*pC && cj5__isspace(*pC)) {
            pC++;
        }

        if (*pC == 0) {
            *ofloat = (double)tmp / (double)divisor;
            return true;
        }
    }

    *ofloat = strtod(str, NULL);
    return true;
}

int cj5_seek_recursive(cj5_result* r, int parent_id, const char* key)
{
    CJ5_ASSERT(parent_id >= 0 && parent_id < r->num_tokens);

    uint32_t key_hash = cj5__hash_fnv32(key, key + cj5__strlen(key));
    return cj5__seek_recursive(r, parent_id, key_hash);
}

int cj5_seek_hash(cj5_result* r, int parent_id, const uint32_t key_hash)
{
    CJ5_ASSERT(parent_id >= 0 && parent_id < r->num_tokens);
    const cj5_token* parent_tok = &r->tokens[parent_id];

    for (int i = parent_id + 1, count = 0; i < r->num_tokens && count < parent_tok->size; i++) {
        const cj5_token* tok = &r->tokens[i];

        if (tok->size != 1 || tok->type != CJ5_TOKEN_STRING) {
            continue;
        }

        if (parent_id == tok->parent_id) {
            if (key_hash == tok->key_hash) {
                CJ5_ASSERT((i + 1) < r->num_tokens);
                return i + 1;    // return next "value" token (array/objects and primitive values)
            }
            count++;
        }
    }

    return -1;
}

int cj5_seek(cj5_result* r, int parent_id, const char* key)
{
    CJ5_ASSERT(parent_id >= 0 && parent_id < r->num_tokens);

    uint32_t key_hash = cj5__hash_fnv32(key, key + cj5__strlen(key));

    return cj5_seek_hash(r, parent_id, key_hash);
}

const char* cj5_get_string(cj5_result* r, int id, char* str, int max_str)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_STRING);
    return cj5__strcpy(str, max_str, &r->json5[tok->start], tok->end - tok->start);
}

double cj5_get_double(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_NUMBER);
    char snum[32];
    double num;
    cj5__strcpy(snum, sizeof(snum), &r->json5[tok->start], tok->end - tok->start);
    bool valid = cj5__tofloat(snum, &num);
    CJ5__UNUSED(valid);
    CJ5_ASSERT(valid);
    return num;
}

float cj5_get_float(cj5_result* r, int id)
{
    return (float)cj5_get_double(r, id);
}

int cj5_get_int(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_NUMBER);
    char snum[32];
    cj5__strcpy(snum, sizeof(snum), &r->json5[tok->start], tok->end - tok->start);
    return (int)strtol(snum, NULL, tok->num_type != CJ5_TOKEN_NUMBER_HEX ? 10 : 16);
}

uint32_t cj5_get_uint(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_NUMBER);
    char snum[32];
    cj5__strcpy(snum, sizeof(snum), &r->json5[tok->start], tok->end - tok->start);
    return (uint32_t)strtoul(snum, NULL, tok->num_type != CJ5_TOKEN_NUMBER_HEX ? 10 : 16);
}

uint64_t cj5_get_uint64(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_NUMBER);
    char snum[64];
    cj5__strcpy(snum, sizeof(snum), &r->json5[tok->start], tok->end - tok->start);
    return strtoull(snum, NULL, tok->num_type != CJ5_TOKEN_NUMBER_HEX ? 10 : 16);
}

int64_t cj5_get_int64(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_NUMBER);
    char snum[64];
    cj5__strcpy(snum, sizeof(snum), &r->json5[tok->start], tok->end - tok->start);
    return strtoll(snum, NULL, tok->num_type != CJ5_TOKEN_NUMBER_HEX ? 10 : 16);
}

bool cj5_get_bool(cj5_result* r, int id)
{
    CJ5_ASSERT(id >= 0 && id < r->num_tokens);
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_BOOL);
    CJ5_ASSERT((tok->end - tok->start) >= 4);

    uint32_t fourcc = *((const uint32_t*)&r->json5[tok->start]);
    if (fourcc == CJ5__TRUE_FOURCC) {
        return true;
    } else if (fourcc == CJ5__FALSE_FOURCC) {
        return false;
    } else {
        CJ5_ASSERT(0);
        return false;
    }
}

double cj5_seekget_double(cj5_result* r, int parent_id, const char* key, double def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_double(r, id) : def_val;
}

float cj5_seekget_float(cj5_result* r, int parent_id, const char* key, float def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_float(r, id) : def_val;
}

int cj5_seekget_int(cj5_result* r, int parent_id, const char* key, int def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_int(r, id) : def_val;
}

uint32_t cj5_seekget_uint(cj5_result* r, int parent_id, const char* key, uint32_t def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_uint(r, id) : def_val;
}

uint64_t cj5_seekget_uint64(cj5_result* r, int parent_id, const char* key, uint64_t def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_uint64(r, id) : def_val;
}

int64_t cj5_seekget_int64(cj5_result* r, int parent_id, const char* key, int64_t def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_int64(r, id) : def_val;
}

bool cj5_seekget_bool(cj5_result* r, int parent_id, const char* key, bool def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_bool(r, id) : def_val;
}

const char* cj5_seekget_string(cj5_result* r, int parent_id, const char* key, char* str,
                               int max_str, const char* def_val)
{
    int id = cj5_seek(r, parent_id, key);
    return id > -1 ? cj5_get_string(r, id, str, max_str) : def_val;
}

int cj5_seekget_array_double(cj5_result* r, int parent_id, const char* key, double* values,
                             int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_double(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_float(cj5_result* r, int parent_id, const char* key, float* values,
                            int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_float(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_int16(cj5_result* r, int parent_id, const char* key, int16_t* values,
                            int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = (int16_t)cj5_get_int(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_uint16(cj5_result* r, int parent_id, const char* key, uint16_t* values,
                             int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = (uint16_t)cj5_get_int(r, i);
        }
        return count;
    } else {
        return 0;
    }
}


int cj5_seekget_array_int(cj5_result* r, int parent_id, const char* key, int* values,
                          int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_int(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_uint(cj5_result* r, int parent_id, const char* key, uint32_t* values,
                           int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_uint(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_uint64(cj5_result* r, int parent_id, const char* key, uint64_t* values,
                             int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_uint64(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_int64(cj5_result* r, int parent_id, const char* key, int64_t* values,
                            int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_int64(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_bool(cj5_result* r, int parent_id, const char* key, bool* values,
                           int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            values[count++] = cj5_get_bool(r, i);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_seekget_array_string(cj5_result* r, int parent_id, const char* key, char** strs,
                             int max_str, int max_values)
{
    int id = key != NULL ? cj5_seek(r, parent_id, key) : parent_id;
    if (id != -1) {
        const cj5_token* tok = &r->tokens[id];
        CJ5__UNUSED(tok);
        CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
        int count = 0;
        for (int i = id + 1, ic = r->num_tokens; i < ic && r->tokens[i].parent_id == id && count < max_values; i++) {
            cj5_get_string(r, i, strs[count++], max_str);
        }
        return count;
    } else {
        return 0;
    }
}

int cj5_get_array_elem(cj5_result* r, int id, int index)
{
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
    CJ5_ASSERT(index < tok->size);
    for (int i = id + 1, count = 0, ic = r->num_tokens; i < ic && count < tok->size; i++) {
        if (r->tokens[i].parent_id == id) {
            if (count == index) {
                return i;
            }
            count++;
        }
    }
    return -1;
}

int cj5_get_array_elem_incremental(cj5_result* r, int id, int index, int prev_elem)
{
    const cj5_token* tok = &r->tokens[id];
    CJ5_ASSERT(tok->type == CJ5_TOKEN_ARRAY);
    CJ5_ASSERT(index < tok->size);
	int start = prev_elem <= 0 ? (id + 1) : (prev_elem + 1);
    for (int i = start, count = index, ic = r->num_tokens; i < ic && count < tok->size; i++) {
        if (r->tokens[i].parent_id == id) {
            if (count == index) {
                return i;
            }
            count++;
        }
    }
    return -1;
}

#    endif    // CJ5_TOKEN_HELPERS
#endif        // CJ5_IMPLEMENT
