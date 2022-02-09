// MIT License
//
// Copyright (c) 2020 Sepehr Taghdisian
// Copyright (c) 2022 Julius Pfrommer
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

#include <string.h>

#if defined(_DEBUG) || defined(NDEBUG)
# include <assert.h>
# define CJ5_ASSERT(_e) assert(_e)
#else
# define CJ5_ASSERT(_e)
#endif

#if defined(_MSC_VER)
# define CJ5__RESTRICT __restrict
# define CJ5_INLINE __inline
#else
# define CJ5__RESTRICT __restrict__
# define CJ5_INLINE inline
#endif

// Max nesting depth of objects and arrays
#define CJ5_MAX_NESTING 32

#define CJ5__FOURCC(_a, _b, _c, _d)                         \
    (((uint32_t)(_a) | ((uint32_t)(_b) << 8) |              \
      ((uint32_t)(_c) << 16) | ((uint32_t)(_d) << 24)))

static const uint32_t CJ5__NULL_FOURCC  = CJ5__FOURCC('n', 'u', 'l', 'l');
static const uint32_t CJ5__TRUE_FOURCC  = CJ5__FOURCC('t', 'r', 'u', 'e');
static const uint32_t CJ5__FALSE_FOURCC = CJ5__FOURCC('f', 'a', 'l', 's');

typedef struct cj5__parser {
    unsigned int pos;
    unsigned int line_start;
    unsigned int line;

    unsigned int next_id;
    unsigned int super_id; // the current parent

    const char* json5;
    unsigned int len;

    cj5_token* tokens;
    unsigned int max_tokens;

    cj5_error_code error;
} cj5__parser;

static CJ5_INLINE bool
cj5__isrange(char ch, char from, char to) {
    return (uint8_t)(ch - from) <= (uint8_t)(to - from);
}

static CJ5_INLINE bool
cj5__isupperchar(char ch) {
    return cj5__isrange(ch, 'A', 'Z');
}

static CJ5_INLINE bool
cj5__islowerchar(char ch) {
    return cj5__isrange(ch, 'a', 'z');
}

static CJ5_INLINE bool
cj5__isnum(char ch) {
    return cj5__isrange(ch, '0', '9');
}

static CJ5_INLINE cj5_token *
cj5__alloc_token(cj5__parser *parser) {
    cj5_token* token = NULL;
    if(parser->next_id < parser->max_tokens) {
        token = &parser->tokens[parser->next_id];
        memset(token, 0x0, sizeof(cj5_token));
    } else {
        parser->error = CJ5_ERROR_OVERFLOW;
    }

    // Always increase the index. So we know eventually how many token would be
    // required (if there are not enough).
    parser->next_id++; 
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
                token->end = parser->pos;
                token->parent_id = parser->super_id;
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
    // We have to use memcpy here or we can get unaligned accesses
    uint32_t fourcc = 0;
    if(start + 4 < len)
        memcpy(&fourcc, &json5[start], 4);
    
    cj5_token_type type;
    if(fourcc == CJ5__NULL_FOURCC) {
        type = CJ5_TOKEN_NULL;
        parser->pos += 4;
    } else if(fourcc == CJ5__TRUE_FOURCC) {
        type = CJ5_TOKEN_BOOL;
        parser->pos += 4;
    } else if(fourcc == CJ5__FALSE_FOURCC) {
        // "false" has five characters
        type = CJ5_TOKEN_BOOL;
        if(start + 5 >= len || json5[start+5] != 'e') {
            parser->error = CJ5_ERROR_INVALID;
            return;
        }
        parser->pos += 5;
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
                parser->pos--; // Reparse this character for the next token
                break;
            }
        }
    }

    cj5_token *token = cj5__alloc_token(parser);
    if(token) {
        token->type = type;
        token->start = start;
        token->end = parser->pos + 1;
        token->parent_id = parser->super_id;
    }
}

static void
cj5__parse_key(cj5__parser* parser) {
    const char* json5 = parser->json5;
    unsigned int start = parser->pos;
    cj5_token* token;

    // A normal string
    if(json5[start] == '\"' || json5[start] == '\'') {
        cj5__parse_string(parser);
        return;
    }

    // An unquoted key.
    // Must start with a-ZA-Z_$.
    // Can contain numbers later on.
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
    if(parser->pos == start) {
        parser->error = CJ5_ERROR_INVALID;
        return;
    }

    // Reparse the character that is beyond the string
    parser->pos--;

    token = cj5__alloc_token(parser);
    if(token) {
        token->type = CJ5_TOKEN_STRING;
        token->start = start;
        token->end = parser->pos + 1;
        token->parent_id = parser->super_id;
    }
}

static void
cj5__skip_comment(cj5__parser* parser) {
    const char* json5 = parser->json5;
    for(; parser->pos < parser->len; parser->pos++) {
        if(json5[parser->pos] == '\n' || json5[parser->pos] == '\r')
            return;
    }
}

static void
cj5__skip_multiline_comment(cj5__parser *parser) {
    const char* json5 = parser->json5;
    unsigned int len = parser->len;
    for(; parser->pos < len; parser->pos++) {
        if(json5[parser->pos] == '*' && parser->pos + 1 < len &&
           json5[parser->pos+1] == '/')
            return;
    }
    parser->error = CJ5_ERROR_INCOMPLETE;
}

cj5_result
cj5_parse(const char *json5, unsigned int len,
          cj5_token *tokens, unsigned int max_tokens) {
    cj5_result r;
    cj5__parser parser;
    memset(&parser, 0x0, sizeof(parser));
    parser.super_id = 0;
    parser.json5 = json5;
    parser.len = len;
    parser.tokens = tokens;
    parser.max_tokens = max_tokens;

    // The nesting depth zero means "outside the root object"
    unsigned short depth = 0;
    char nesting[CJ5_MAX_NESTING]; // Contains either '\0', '{' or '[' for the
                                   // type of nesting at each depth. '\0'
                                   // indicates we are out of the root object.
    char slot[CJ5_MAX_NESTING];    // Next content to parse: 'k' (key), ':', 'v'
                                   // (value) or ',' (komma).
    nesting[0] = 0; // Becomes '{' if there is a virtual root object
    slot[0] = 0;
    bool before_root = true;
    cj5_token *token;

    for(; parser.pos < len; parser.pos++) {
        char c = json5[parser.pos];
        switch(c) {
        case '\n': // Newline
            parser.line++;
            parser.line_start = parser.pos;
            break;

        case '\r': // Whitespace
        case '\t':
        case ' ':
            break;

        case '#': // Comment
        case '/':
            if(json5[parser.pos] == '#') {
                cj5__skip_comment(&parser);
            } else {
                if(parser.pos + 1 >= parser.len) {
                    parser.error = CJ5_ERROR_INVALID;
                } else if(json5[parser.pos + 1] == '/') {
                    cj5__skip_comment(&parser);
                } else if(json5[parser.pos + 1] == '*') {
                    cj5__skip_multiline_comment(&parser);
                }
            }
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

            // Correct slot?
            if(slot[depth] != 'v') {
                // Start the root object
                if(!before_root || c != '{') {
                    parser.error = CJ5_ERROR_INVALID;
                    goto finish;
                }
                before_root = false;
            }

            depth++; // Increase the nesting
            nesting[depth] = c;
            slot[depth] = (c == '{') ? 'k' : 'v';

            // Create a token for the object or array
            token = cj5__alloc_token(&parser);
            if(token) {
                token->parent_id = parser.super_id;
                token->type = (c == '{') ? CJ5_TOKEN_OBJECT : CJ5_TOKEN_ARRAY;
                token->start = parser.pos;
                token->size = parser.next_id; // Adjusted when closed
                parser.super_id = parser.next_id - 1; // The new super_id is for this token
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
               (c == '}' && slot[depth] != 'k' && slot[depth] != ',')) {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }

            // Finish setting the super-token
            if(parser.error != CJ5_ERROR_OVERFLOW) {
                token = &tokens[parser.super_id];
                token->end = parser.pos + 1;
                token->size++; // End of the object -> count as one value for the parent
                parser.super_id = token->parent_id; // Reset the super_id
            }

            // Step one level up
            depth--;
            slot[depth] = (depth > 0) ? ',' : 0; // zero if we step out the root object
            break;

        case ':': // Colon (between key and value)
            if(slot[depth] != ':') {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }
            slot[depth] = 'v';
            break;

        case ',': // Comma
            if(slot[depth] != ',') {
                parser.error = CJ5_ERROR_INVALID;
                goto finish;
            }
            slot[depth] = (nesting[depth] == '{') ? 'k' : 'v';
            break;

        default: // Value
            if(slot[depth] == 'v') {
                cj5__parse_primitive(&parser); // Parse primitive value
                token->size++; // One more value for the parent
                slot[depth] = ',';
            } else if(slot[depth] == 'k') {
                cj5__parse_key(&parser); // Parse key
                token->size++; // Keys count towards the length
                slot[depth] = ':';
            } else {
                if(before_root) {
                    // Create a virtual root object
                    CJ5_ASSERT(depth == 0);
                    nesting[0] = '{';
                    slot[0] = 'k';
                    token = cj5__alloc_token(&parser);
                    if(token) {
                        token->parent_id = parser.super_id;
                        token->type = CJ5_TOKEN_OBJECT;
                        token->start = parser.pos;
                        token->size = parser.next_id;
                        parser.super_id = parser.next_id - 1;
                    }
                    parser.pos--; // Reparse the current character
                } else {
                    parser.error = CJ5_ERROR_INVALID;
                }
            }
            if(parser.error && parser.error != CJ5_ERROR_OVERFLOW)
                goto finish;
            break;
        }
    }

    // The initial object was not closed
    if(depth != 0) {
        parser.error = CJ5_ERROR_INCOMPLETE;
        goto finish;
    }

    // Close the virtual root object if there is one
    if(nesting[0] == '{' && parser.error != CJ5_ERROR_OVERFLOW) {
        token = &tokens[parser.super_id];
        token->end = parser.pos;
        token->size = parser.next_id - token->size;
        parser.super_id = token->parent_id; // Reset the super_id
    }

 finish:
    memset(&r, 0x0, sizeof(r));
    r.error = parser.error;
    r.error_line = parser.line;
    r.error_col = parser.pos - parser.line_start;
    r.num_tokens = parser.next_id; // How many tokens (would) have been consumed
                                   // by the parser?

    // Set the tokens and original string only if successfully parsed
    if(r.error == CJ5_ERROR_NONE) {
        r.tokens = tokens;
        r.json5 = json5;
    }

    return r;
}

cj5_error_code
cj5_get_float(const char *json5, const cj5_token *token, double *out) {
    size_t parsed = parseDouble(&json5[token->start],
                                token->end - token->start, out);
    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

cj5_error_code
cj5_get_int(const char *json5, const cj5_token *token, int64_t *out) {
    size_t parsed = parseInt64(&json5[token->start],
                               token->end - token->start, out);
    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

cj5_error_code
cj5_get_uint(const char *json5, const cj5_token *token, uint64_t *out) {
    size_t parsed = parseUInt64(&json5[token->start],
                                token->end - token->start, out);
    return (parsed != 0) ? CJ5_ERROR_NONE : CJ5_ERROR_INVALID;
}

cj5_error_code
cj5_get_str(const char *json5, const cj5_token *token,
            char *buf, unsigned *buflen) {
    const char *pos = &json5[token->start];
    const char *end = &json5[token->end];
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
            case 'u': // The next four characters are an utf8 code
                if(pos + 4 >= end)
                    return CJ5_ERROR_INCOMPLETE;
                
                // Parse the unicode code point
                uint32_t utf = 0;
                for(unsigned int i = 0; i < 4; i++) {
                    pos++;
                    uint8_t byte = (uint8_t)*pos;
                    if(byte >= '0' && byte <= '9') {
                        byte = byte - (uint8_t)'0';
                    } else if(byte >= 'a' && byte <='f') {
                        byte = byte - (uint8_t)('a' - 10);
                    } else if(byte >= 'A' && byte <='F') {
                        byte = byte - (uint8_t)('A' - 10);
                    } else {
                        return CJ5_ERROR_INVALID;
                    }
                    utf = (utf << 4) | (byte & 0xF);
                }
                
                // Print the utf8 bytes
                if(utf <= 0x7F) { // Plain ASCII
                    buf[outpos++] = (char)utf;
                } else if(utf <= 0x07FF) { // 2-byte unicode
                    buf[outpos++] = (char) (((utf >> 6) & 0x1F) | 0xC0);
                    buf[outpos++] = (char) (((utf >> 0) & 0x3F) | 0x80);
                } else if(utf <= 0xFFFF) { // 3-byte unicode
                    buf[outpos++] = (char) (((utf >> 12) & 0x0F) | 0xE0);
                    buf[outpos++] = (char) (((utf >>  6) & 0x3F) | 0x80);
                    buf[outpos++] = (char) (((utf >>  0) & 0x3F) | 0x80);
                } else if(utf <= 0x10FFFF) { // 4-byte unicode
                    buf[outpos++] = (char) (((utf >> 18) & 0x07) | 0xF0);
                    buf[outpos++] = (char) (((utf >> 12) & 0x3F) | 0x80);
                    buf[outpos++] = (char) (((utf >>  6) & 0x3F) | 0x80);
                    buf[outpos++] = (char) (((utf >>  0) & 0x3F) | 0x80);
                } else {
                    return CJ5_ERROR_INVALID; // Not a utf8 string
                }
                break;
            default:
                return CJ5_ERROR_INVALID;
            }
            continue;
        }

        // Unprintable ascii character
        if(c < 32 || c == 127)
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
