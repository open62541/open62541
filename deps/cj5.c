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

#include "cj5.h"
#include "parse_num.h"

#include <math.h>
#include <float.h>
#include <string.h>

#if defined(_MSC_VER)
# define CJ5_INLINE __inline
#else
# define CJ5_INLINE inline
#endif

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((double)(INFINITY-INFINITY))
#endif

#if defined(_MSC_VER)
# pragma warning(disable: 4056)
# pragma warning(disable: 4756)
#endif

/* Max nesting depth of objects and arrays */
#define CJ5_MAX_NESTING 32

#define CJ5__FOURCC(_a, _b, _c, _d)                         \
    (((uint32_t)(_a) | ((uint32_t)(_b) << 8) |              \
      ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

static const uint32_t CJ5__NULL_FOURCC  = CJ5__FOURCC('n', 'u', 'l', 'l');
static const uint32_t CJ5__TRUE_FOURCC  = CJ5__FOURCC('t', 'r', 'u', 'e');
static const uint32_t CJ5__FALSE_FOURCC = CJ5__FOURCC('f', 'a', 'l', 's');

typedef struct {
    unsigned int pos;
    unsigned int line_start;
    unsigned int line;
    cj5_error_code error;

    const char *json5;
    unsigned int len;

    unsigned int curr_tok_idx;

    cj5_token *tokens;
    unsigned int token_count;
    unsigned int max_tokens;

    bool stop_early;
} cj5__parser;

static CJ5_INLINE bool
cj5__isrange(char ch, char from, char to) {
    return (uint8_t)(ch - from) <= (uint8_t)(to - from);
}

#define cj5__isupperchar(ch) cj5__isrange(ch, 'A', 'Z')
#define cj5__islowerchar(ch) cj5__isrange(ch, 'a', 'z')
#define cj5__isnum(ch)       cj5__isrange(ch, '0', '9')

static cj5_token *
cj5__alloc_token(cj5__parser *parser) {
    cj5_token* token = NULL;
    if(parser->token_count < parser->max_tokens) {
        token = &parser->tokens[parser->token_count];
        memset(token, 0x0, sizeof(cj5_token));
    } else {
        parser->error = CJ5_ERROR_OVERFLOW;
    }

    // Always increase the index. So we know eventually how many token would be
    // required (if there are not enough).
    parser->token_count++;
    return token;
}

static void
cj5__parse_string(cj5__parser *parser) {
    const char *json5 = parser->json5;
    unsigned int len = parser->len;
    unsigned int start = parser->pos;
    char str_open = json5[start];

    parser->pos++;
    for(; parser->pos < len; parser->pos++) {
        char c = json5[parser->pos];

        // End of string
        if(str_open == c) {
            cj5_token *token = cj5__alloc_token(parser);
            if(token) {
                token->type = CJ5_TOKEN_STRING;
                token->start = start + 1;
                token->end = parser->pos - 1;
                token->size = token->end - token->start + 1;
                token->parent_id = parser->curr_tok_idx;
            } 
            return;
        }

        // Unescaped newlines are forbidden
        if(c == '\n') {
            parser->error = CJ5_ERROR_INVALID;
            return;
        }

        // Escape char
        if(c == '\\') {
            if(parser->pos + 1 >= len) {
                parser->error = CJ5_ERROR_INCOMPLETE;
                return;
            }
            parser->pos++;
            switch(json5[parser->pos]) {
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            case 'u': // The next four characters are an utf8 code
                parser->pos++;
                if(parser->pos + 4 >= len) {
                    parser->error = CJ5_ERROR_INVALID;
                    return;
                }
                for(unsigned int i = 0; i < 4; i++) {
                    // If it isn't a hex character we have an error
                    if(!(json5[parser->pos] >= 48 && json5[parser->pos] <= 57) && /* 0-9 */
                       !(json5[parser->pos] >= 65 && json5[parser->pos] <= 70) && /* A-F */
                       !(json5[parser->pos] >= 97 && json5[parser->pos] <= 102))  /* a-f */
                        {
                            parser->error = CJ5_ERROR_INVALID;
                            return;
                        }
                    parser->pos++;
                }
                parser->pos--;
                break;
            case '\n': // Escape break line
                parser->line++;
                parser->line_start = parser->pos;
                break;
            default:
                parser->error = CJ5_ERROR_INVALID;
                return;
            }
        }
    }

    // The file has ended before the string terminates
    parser->error = CJ5_ERROR_INCOMPLETE;
}

// parser->pos is advanced a last time in the next iteration of the main
// parse-loop. So we leave parse-primitive in a state where parse->pos points to
// the last character of the primitive value (or the quote-character of the
// string).
static void
cj5__parse_primitive(cj5__parser* parser) {
    const char* json5 = parser->json5;
    unsigned int len = parser->len;
    unsigned int start = parser->pos;

    // String value
    if(json5[start] == '\"' ||
       json5[start] == '\'') {
        cj5__parse_string(parser);
        return;
    }

    // Fast comparison of bool, and null.
    // Make the comparison case-insensitive.
    uint32_t fourcc = 0;
    if(start + 3 < len) {
        fourcc += json5[start] | 32;
        fourcc += (json5[start+1] | 32) << 8;
        fourcc += (json5[start+2] | 32) << 16;
        fourcc += (json5[start+3] | 32) << 24;
    }
    
    cj5_token_type type;
    if(fourcc == CJ5__NULL_FOURCC) {
        type = CJ5_TOKEN_NULL;
        parser->pos += 3;
    } else if(fourcc == CJ5__TRUE_FOURCC) {
        type = CJ5_TOKEN_BOOL;
        parser->pos += 3;
    } else if(fourcc == CJ5__FALSE_FOURCC) {
        // "false" has five characters
        type = CJ5_TOKEN_BOOL;
        if(start + 4 >= len || (json5[start+4] | 32) != 'e') {
            parser->error = CJ5_ERROR_INVALID;
            return;
        }
        parser->pos += 4;
    } else {
        // Numbers are checked for basic compatibility.
        // But they are fully parsed only in the cj5_get_XXX functions.
        type = CJ5_TOKEN_NUMBER;
        for(; parser->pos < len; parser->pos++) {
            if(!cj5__isnum(json5[parser->pos]) &&
               !(json5[parser->pos] == '.') &&
               !cj5__islowerchar(json5[parser->pos]) && 
               !cj5__isupperchar(json5[parser->pos]) &&
               !(json5[parser->pos] == '+') && !(json5[parser->pos] == '-')) {
                break;
            }
        }
        parser->pos--; // Point to the last character that is still inside the
                       // primitive value
    }

    cj5_token *token = cj5__alloc_token(parser);
    if(token) {
        token->type = type;
        token->start = start;
        token->end = parser->pos;
        token->size = parser->pos - start + 1;
        token->parent_id = parser->curr_tok_idx;
    }
}

static void
cj5__parse_key(cj5__parser* parser) {
    const char* json5 = parser->json5;
    unsigned int start = parser->pos;
    cj5_token* token;

    // Key is a a normal string
    if(json5[start] == '\"' || json5[start] == '\'') {
        cj5__parse_string(parser);
        return;
    }

    // An unquoted key. Must start with a-ZA-Z_$. Can contain numbers later on.
    unsigned int len = parser->len;
    for(; parser->pos < len; parser->pos++) {
        if(cj5__islowerchar(json5[parser->pos]) ||
           cj5__isupperchar(json5[parser->pos]) ||
           json5[parser->pos] == '_' || json5[parser->pos] == '$')
            continue;
        if(cj5__isnum(json5[parser->pos]) && parser->pos != start)
            continue;
        break;
    }

    // An empty key is not allowed
    if(parser->pos <= start) {
        parser->error = CJ5_ERROR_INVALID;
        return;
    }

    // Move pos to the last character within the unquoted key
    parser->pos--;

    token = cj5__alloc_token(parser);
    if(token) {
        token->type = CJ5_TOKEN_STRING;
        token->start = start;
        token->end = parser->pos;
        token->size = parser->pos - start + 1;
        token->parent_id = parser->curr_tok_idx;
    }
}

static void
cj5__skip_comment(cj5__parser* parser) {
    const char* json5 = parser->json5;

    // Single-line comment
    if(json5[parser->pos] == '#') {
    skip_line:
        while(parser->pos < parser->len) {
            if(json5[parser->pos] == '\n') {
                parser->pos--; // Reparse the newline in the main parse loop
                return;
            }
            parser->pos++;
        }
        return;
    }

    // Comment begins with '/' but not enough space for another character
    if(parser->pos + 1 >= parser->len) {
        parser->error = CJ5_ERROR_INVALID;
        return;
    }
    parser->pos++;

    // Comment begins with '//' -> single-line comment
    if(json5[parser->pos] == '/')
        goto skip_line;

    // Multi-line comments begin with '/*' and end with '*/'
    if(json5[parser->pos] == '*') {
        parser->pos++;
        for(; parser->pos + 1 < parser->len; parser->pos++) {
            if(json5[parser->pos] == '*' && json5[parser->pos + 1] == '/') {
                parser->pos++;
                return;
            }
            // Remember we passed a newline
            if(json5[parser->pos] == '\n') {
                parser->line++;
                parser->line_start = parser->pos;
            }
        }
    }

    // Unknown comment type or the multi-line comment is not terminated
    parser->error = CJ5_ERROR_INCOMPLETE;
}

cj5_result
cj5_parse(const char *json5, unsigned int len,
          cj5_token *tokens, unsigned int max_tokens,
          cj5_options *options) {
    cj5_result r;
    cj5__parser parser;
    memset(&parser, 0x0, sizeof(parser));
    parser.curr_tok_idx = 0;
    parser.json5 = json5;
    parser.len = len;
    parser.tokens = tokens;
    parser.max_tokens = max_tokens;

    if(options)
        parser.stop_early = options->stop_early;

    unsigned short depth = 0; // Nesting depth zero means "outside the root object"
    char nesting[CJ5_MAX_NESTING]; // Contains either '\0', '{' or '[' for the
                                   // type of nesting at each depth. '\0'
                                   // indicates we are out of the root object.
    char next[CJ5_MAX_NESTING];    // Next content to parse: 'k' (key), ':', 'v'
                                   // (value) or ',' (comma).
    next[0] = 'v';  // The root is a "value" (object, array or primitive). If we
                    // detect a colon after the first value then everything is
                    // wrapped into a "virtual root object" and the parsing is
                    // restarted.
    nesting[0] = 0; // Becomes '{' if there is a virtual root object

    cj5_token *token = NULL; // The current token

 start_parsing:
    for(; parser.pos < len; parser.pos++) {
        char c = json5[parser.pos];
        switch(c) {
        case '\n': // Skip newline
            parser.line++;
            parser.line_start = parser.pos;
            break;

        case '\r': // Skip whitespace
        case '\t':
        case ' ':
            break;

        case '#': // Skip comment
        case '/':
            cj5__skip_comment(&parser);
            if(parser.error != CJ5_ERROR_NONE &&
               parser.error != CJ5_ERROR_OVERFLOW)
                goto finish;
            break;

        case '{': // Open an object or array
        case '[':
            // Check the nesting depth
            if(depth + 1 >= CJ5_MAX_NESTING) {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }

            // Correct next?
            if(next[depth] != 'v') {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }

            depth++; // Increase the nesting depth
            nesting[depth] = c; // Set the nesting type
            next[depth] = (c == '{') ? 'k' : 'v'; // next is either a key or a value

            // Create a token for the object or array
            token = cj5__alloc_token(&parser);
            if(token) {
                token->parent_id = parser.curr_tok_idx;
                token->type = (c == '{') ? CJ5_TOKEN_OBJECT : CJ5_TOKEN_ARRAY;
                token->start = parser.pos;
                token->size = 0;
                parser.curr_tok_idx = parser.token_count - 1; // The new curr_tok_idx
                                                              // is for this token
            }
            break;

        case '}': // Close an object or array
        case ']':
            // Check the nesting depth. Note that a "virtual root object" at
            // depth zero must not be closed.
            if(depth == 0) {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }

            // Check and adjust the nesting. Note that ']' - '[' == 2 and '}' -
            // '{' == 2. Arrays can always be closed. Objects can only close
            // when a key or a comma is expected.
            if(c - nesting[depth] != 2 ||
               (c == '}' && next[depth] != 'k' && next[depth] != ',')) {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }

            if(token) {
                // Finalize the current token
                token->end = parser.pos;

                // Move to the parent and increase the parent size. Omit this
                // when we leave the root (parent the same as the current
                // token).
                if(parser.curr_tok_idx != token->parent_id) {
                    parser.curr_tok_idx = token->parent_id;
                    token = &tokens[token->parent_id];
                    token->size++;
                }
            }

            // Step one level up
            depth--;
            next[depth] = (depth == 0) ? 0 : ','; // zero if we step out the root
                                                  // object. then we do not look for
                                                  // another element.

            // The first element was successfully parsed. Stop early or try to
            // parse the full input string?
            if(depth == 0 && parser.stop_early)
                goto finish;

            break;

        case ':': // Colon (between key and value)
            if(next[depth] != ':') {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }
            next[depth] = 'v';
            break;

        case ',': // Comma
            if(next[depth] != ',') {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }
            next[depth] = (nesting[depth] == '{') ? 'k' : 'v';
            break;

        default: // Value or key
            if(next[depth] == 'v') {
                cj5__parse_primitive(&parser); // Parse primitive value
                if(nesting[depth] != 0) {
                    // Parent is object or array
                    if(token)
                        token->size++;
                    next[depth] = ',';
                } else {
                    // The current value was the root element. Don't look for
                    // any next element.
                    next[depth] = 0;

                    // The first element was successfully parsed. Stop early or try to
                    // parse the full input string?
                    if(parser.stop_early)
                        goto finish;
                }
            } else if(next[depth] == 'k') {
                cj5__parse_key(&parser);
                if(token)
                    token->size++; // Keys count towards the length
                next[depth] = ':';
            } else {
                parser.error = CJ5_ERROR_INVALID;
            }

            if(parser.error && parser.error != CJ5_ERROR_OVERFLOW)
                goto finish;

            break;
        }
    }

    // Are we back to the initial nesting depth?
    if(depth != 0) {
        parser.error = CJ5_ERROR_INCOMPLETE;
        goto finish;
    }

    // Close the virtual root object if there is one
    if(nesting[0] == '{' && parser.error != CJ5_ERROR_OVERFLOW) {
        // Check the we end after a complete key-value pair (or dangling comma)
        if(next[0] != 'k' && next[0] != ',')
            parser.error = CJ5_ERROR_INVALID;
        tokens[0].end = parser.pos - 1;
    }

 finish:
    // If parsing failed at the initial nesting depth, create a virtual root object
    // and restart parsing.
    if(parser.error != CJ5_ERROR_NONE &&
       parser.error != CJ5_ERROR_OVERFLOW &&
       depth == 0 && nesting[0] != '{') {
        parser.token_count = 0;
        token = cj5__alloc_token(&parser);
        if(token) {
            token->parent_id = 0;
            token->type = CJ5_TOKEN_OBJECT;
            token->start = 0;
            token->size = 0;

            nesting[0] = '{';
            next[0] = 'k';

            parser.curr_tok_idx = 0;
            parser.pos = 0;
            parser.error = CJ5_ERROR_NONE;
            goto start_parsing;
        }
    }

    memset(&r, 0x0, sizeof(r));
    r.error = parser.error;
    r.error_line = parser.line;
    r.error_col = parser.pos - parser.line_start;
    r.num_tokens = parser.token_count; // How many tokens (would) have been
                                       // consumed by the parser?

    // Not a single token was parsed -> return an error
    if(r.num_tokens == 0)
        r.error = CJ5_ERROR_INCOMPLETE;

    // Set the tokens and original string only if successfully parsed
    if(r.error == CJ5_ERROR_NONE) {
        r.tokens = tokens;
        r.json5 = json5;
    }

    return r;
}

cj5_error_code
cj5_get_bool(const cj5_result *r, unsigned int tok_index, bool *out) {
    const cj5_token *token = &r->tokens[tok_index];
    if(token->type != CJ5_TOKEN_BOOL)
        return CJ5_ERROR_INVALID;
    *out = (r->json5[token->start] == 't');
    return CJ5_ERROR_NONE;
}

cj5_error_code
cj5_get_float(const cj5_result *r, unsigned int tok_index, double *out) {
    const cj5_token *token = &r->tokens[tok_index];
    if(token->type != CJ5_TOKEN_NUMBER)
        return CJ5_ERROR_INVALID;

    const char *tokstr = &r->json5[token->start];
    size_t toksize = token->end - token->start + 1;
    if(toksize == 0)
        return CJ5_ERROR_INVALID;

    // Skip prefixed +/-
    bool neg = false;
    if(tokstr[0] == '+' || tokstr[0] == '-') {
        neg = (tokstr[0] == '-');
        tokstr++;
        toksize--;
    }

    // Detect prefixed inf/nan
    if(strncmp(tokstr, "Infinity", toksize) == 0) {
        *out = neg ? -INFINITY : INFINITY;
        return CJ5_ERROR_NONE;
    } else if(strncmp(tokstr, "NaN", toksize) == 0) {
        *out = NAN;
        return CJ5_ERROR_NONE;
    }

    // reset the +/- detection and parse
    tokstr = &r->json5[token->start];
    toksize = token->end - token->start + 1;
    size_t parsed = parseDouble(tokstr, toksize, out);

    // There must only be whitespace between the end of the parsed number and
    // the end of the token
    for(size_t i = parsed; i < toksize; i++) {
        if(tokstr[i] != ' ' && tokstr[i] -'\t' >= 5)
            return CJ5_ERROR_INVALID;
    }

    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

cj5_error_code
cj5_get_int(const cj5_result *r, unsigned int tok_index,
            int64_t *out) {
    const cj5_token *token = &r->tokens[tok_index];
    if(token->type != CJ5_TOKEN_NUMBER)
        return CJ5_ERROR_INVALID;
    size_t parsed = parseInt64(&r->json5[token->start], token->size, out);
    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

cj5_error_code
cj5_get_uint(const cj5_result *r, unsigned int tok_index,
             uint64_t *out) {
    const cj5_token *token = &r->tokens[tok_index];
    if(token->type != CJ5_TOKEN_NUMBER)
        return CJ5_ERROR_INVALID;
    size_t parsed = parseUInt64(&r->json5[token->start], token->size, out);
    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

static const uint32_t SURROGATE_OFFSET = 0x10000u - (0xD800u << 10) - 0xDC00;

static cj5_error_code
parse_codepoint(const char *pos, uint32_t *out_utf) {
    uint32_t utf = 0;
    for(unsigned int i = 0; i < 4; i++) {
        char byte = pos[i];
        if(cj5__isnum(byte)) {
            byte = (char)(byte - '0');
        } else if(cj5__isrange(byte, 'a', 'f')) {
            byte = (char)(byte - ('a' - 10));
        } else if(cj5__isrange(byte, 'A', 'F')) {
            byte = (char)(byte - ('A' - 10));
        } else {
            return CJ5_ERROR_INVALID;
        }
        utf = (utf << 4) | ((uint8_t)byte & 0xF);
    }
    *out_utf = utf;
    return CJ5_ERROR_NONE;
}

cj5_error_code
cj5_get_str(const cj5_result *r, unsigned int tok_index,
            char *buf, unsigned int *buflen) {
    const cj5_token *token = &r->tokens[tok_index];
    if(token->type != CJ5_TOKEN_STRING)
        return CJ5_ERROR_INVALID;

    const char *pos = &r->json5[token->start];
    const char *end = &r->json5[token->end + 1];
    unsigned int outpos = 0;
    for(; pos < end; pos++) {
        uint8_t c = (uint8_t)*pos;

        // Process an escape character
        if(c == '\\') {
            if(pos + 1 >= end)
                return CJ5_ERROR_INCOMPLETE;
            pos++;
            c = (uint8_t)*pos;
            switch(c) {
            case '\"': buf[outpos++] = '\"'; break;
            case '\\': buf[outpos++] = '\\'; break;
            case '\n': buf[outpos++] = '\n'; break; // escape newline
            case '/':  buf[outpos++] = '/';  break;
            case 'b':  buf[outpos++] = '\b'; break;
            case 'f':  buf[outpos++] = '\f'; break;
            case 'r':  buf[outpos++] = '\r'; break;
            case 'n':  buf[outpos++] = '\n'; break;
            case 't':  buf[outpos++] = '\t'; break;
            case 'u': {
                // Parse the unicode code point
                if(pos + 4 >= end)
                    return CJ5_ERROR_INCOMPLETE;
                pos++;
                uint32_t utf;
                cj5_error_code err = parse_codepoint(pos, &utf);
                if(err != CJ5_ERROR_NONE)
                    return err;
                pos += 3;

                if(0xD800 <= utf && utf <= 0xDBFF) {
                    // Parse a surrogate pair
                    if(pos + 6 >= end)
                        return CJ5_ERROR_INVALID;
                    if(pos[1] != '\\' && pos[3] != 'u')
                        return CJ5_ERROR_INVALID;
                    pos += 3;
                    uint32_t trail;
                    err = parse_codepoint(pos, &trail);
                    if(err != CJ5_ERROR_NONE)
                        return err;
                    pos += 3;
                    utf = (utf << 10) + trail + SURROGATE_OFFSET;
                } else if(0xDC00 <= utf && utf <= 0xDFFF) {
                    // Invalid Unicode '\\u%04X'
                    return CJ5_ERROR_INVALID;
                }
                
                // Write the utf8 bytes of the code point
                if(utf <= 0x7F) { // Plain ASCII
                    buf[outpos++] = (char)utf;
                } else if(utf <= 0x07FF) { // 2-byte unicode
                    buf[outpos++] = (char)(((utf >> 6) & 0x1F) | 0xC0);
                    buf[outpos++] = (char)(((utf >> 0) & 0x3F) | 0x80);
                } else if(utf <= 0xFFFF) { // 3-byte unicode
                    buf[outpos++] = (char)(((utf >> 12) & 0x0F) | 0xE0);
                    buf[outpos++] = (char)(((utf >>  6) & 0x3F) | 0x80);
                    buf[outpos++] = (char)(((utf >>  0) & 0x3F) | 0x80);
                } else if(utf <= 0x10FFFF) { // 4-byte unicode
                    buf[outpos++] = (char)(((utf >> 18) & 0x07) | 0xF0);
                    buf[outpos++] = (char)(((utf >> 12) & 0x3F) | 0x80);
                    buf[outpos++] = (char)(((utf >>  6) & 0x3F) | 0x80);
                    buf[outpos++] = (char)(((utf >>  0) & 0x3F) | 0x80);
                } else {
                    return CJ5_ERROR_INVALID; // Not a utf8 string
                }
                break;
            }
            default:
                return CJ5_ERROR_INVALID;
            }
            continue;
        }

        // Unprintable ascii characters must be escaped. JSON5 allows nested
        // quotes if the quote character is not the same as the surrounding
        // quote character, e.g. 'this is my "quote"'. This logic is in the
        // token parsing code and not in this "string extraction" method.
        if(c < ' '   || c == 127)
            return CJ5_ERROR_INVALID;

        // Ascii character or utf8 byte
        buf[outpos++] = (char)c;
    }

    // Terminate with \0
    buf[outpos] = 0;

    // Set the output length
    if(buflen)
        *buflen = outpos;
    return CJ5_ERROR_NONE;
}

void
cj5_skip(const cj5_result *r, unsigned int *tok_index) {
    unsigned int idx = *tok_index;
    unsigned int end = r->tokens[idx].end;
    do { idx++; } while(idx < r->num_tokens &&
                        r->tokens[idx].start < end);
    *tok_index = idx;
}

cj5_error_code
cj5_find(const cj5_result *r, unsigned int *tok_index,
         const char *key) {
    // It has to be an object
    unsigned int idx = *tok_index;
    if(r->tokens[idx].type != CJ5_TOKEN_OBJECT)
        return CJ5_ERROR_INVALID;
    unsigned int size = r->tokens[idx].size;

    // Skip to the first key
    idx++;

    // Size is number of keys + number of values
    for(unsigned int i = 0; i < size; i += 2) {
        // Key has to be a string
        if(r->tokens[idx].type != CJ5_TOKEN_STRING)
            return CJ5_ERROR_INVALID;

        // Return the index to the value if the key matches
        const char *keystart = &r->json5[r->tokens[idx].start];
        size_t keysize = r->tokens[idx].end - r->tokens[idx].start + 1;
        if(strncmp(key, keystart, keysize) == 0) {
            *tok_index = idx + 1;
            return CJ5_ERROR_NONE;
        }

        // Skip over the value
        idx++;
        cj5_skip(r, &idx);
    }
    return CJ5_ERROR_NOTFOUND;
}
