/**
 * @author (c) Julius Pfrommer
 *             2023, Fraunhofer IOSB, Germany
 * @author (c) Eyal Rozenberg <eyalroz1@gmx.com>
 *             2021-2023, Haifa, Palestine/Israel
 * @author (c) Marco Paland (info@paland.com)
 *             2014-2019, PALANDesign Hannover, Germany
 *
 * @note Others have made smaller contributions to this file: see the
 * contributors page at https://github.com/eyalroz/printf/graphs/contributors
 * or ask one of the authors. The original code for exponential specifiers was
 * contributed by Martijn Jasperse <m.jasperse@gmail.com>.
 *
 * @brief Small stand-alone implementation of the printf family of functions
 * (`(v)printf`, `(v)s(n)printf` etc., geared towards use on embedded systems with
 * limited resources.
 *
 * @note the implementations are thread-safe; re-entrant; use no functions from
 * the standard library; and do not dynamically allocate any memory.
 *
 * @license The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mp_printf.h"
#include "dtoa.h"

#include <stdint.h>
#include <stdbool.h>

// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
#define PRINTF_INTEGER_BUFFER_SIZE    32

// size of the fixed (on-stack) buffer for printing individual decimal numbers.
// this must be big enough to hold one converted floating-point value including
// padded zeros.
#define PRINTF_DECIMAL_BUFFER_SIZE    32

// Default precision for the floating point conversion specifiers (the C
// standard sets this at 6)
#define PRINTF_DEFAULT_FLOAT_PRECISION  6

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
  // Only used with PRINTF_SUPPORT_MSVC_STYLE_INTEGER_SPECIFIERS
#define FLAGS_LONG      (1U <<  9U)
#define FLAGS_LONG_LONG (1U << 10U)
#define FLAGS_PRECISION (1U << 11U)
#define FLAGS_ADAPT_EXP (1U << 12U)
#define FLAGS_POINTER   (1U << 13U)
  // Note: Similar, but not identical, effect as FLAGS_HASH
#define FLAGS_SIGNED    (1U << 14U)

#define BASE_BINARY    2
#define BASE_OCTAL     8
#define BASE_DECIMAL  10
#define BASE_HEX      16

typedef unsigned int printf_flags_t;
typedef uint8_t numeric_base_t;
typedef unsigned long long printf_unsigned_value_t;
typedef long long printf_signed_value_t;

// Note in particular the behavior here on LONG_MIN or LLONG_MIN; it is valid
// and well-defined, but if you're not careful you can easily trigger undefined
// behavior with -LONG_MIN or -LLONG_MIN
#define ABS_FOR_PRINTING(_x)                                            \
    ((printf_unsigned_value_t)((_x) > 0 ? (_x) : -((printf_signed_value_t)_x)))

// internal secure strlen @return The length of the string (excluding the
// terminating 0) limited by 'maxsize' @note strlen uses size_t, but wes only
// use this function with size_t variables - hence the signature.
static size_t
strnlen_s_(const char *str, size_t maxsize) {
    for(size_t i = 0; i < maxsize; i++) {
        if(!str[i])
            return i;
    }
    return maxsize;
}

// internal test if char is a digit (0-9)
// @return true if char is a digit
static bool is_digit_(char ch) { return (ch >= '0') && (ch <= '9'); }

// internal ASCII string to size_t conversion
static size_t
atou_(const char **str) {
    size_t i = 0U;
    while(is_digit_(**str)) {
        i = i * 10U + (size_t)(*((*str)++) - '0');
    }
    return i;
}

// Output buffer
typedef struct {
    char *buffer;
    size_t pos;
    size_t max_chars;
} output_t;

static void
putchar_(output_t *out, char c) {
    size_t write_pos = out->pos++;
    // We're _always_ increasing pos, so as to count how may characters
    // _would_ have been written if not for the max_chars limitation
    if(write_pos >= out->max_chars)
        return;
    // it must be the case that out->buffer != NULL , due to the constraint
    // on output_t ; and note we're relying on write_pos being non-negative.
    out->buffer[write_pos] = c;
}

static void
out_(output_t *out, const char *buf, size_t len) {
    if(out->pos < out->max_chars) {
        size_t write_len = len;
        if(out->pos + len > out->max_chars)
            write_len = out->max_chars - out->pos;
        for(size_t i = 0; i < write_len; i++)
            out->buffer[out->pos + i] = buf[i];
    }
    out->pos += len; // Always increase pos by len
}

// output the specified string in reverse, taking care of any zero-padding
static void
out_rev_(output_t *output, const char *buf, size_t len, size_t width,
         printf_flags_t flags) {
    const size_t start_pos = output->pos;

    // pad spaces up to given width
    if(!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
        for(size_t i = len; i < width; i++) {
            putchar_(output, ' ');
        }
    }

    // reverse string
    while(len) {
        putchar_(output, buf[--len]);
    }

    // append pad spaces up to given width
    if(flags & FLAGS_LEFT) {
        while(output->pos - start_pos < width) {
            putchar_(output, ' ');
        }
    }
}

// Invoked by print_integer after the actual number has been printed, performing
// necessary work on the number's prefix (as the number is initially printed in
// reverse order)
static void
print_integer_finalization(output_t *output, char *buf, size_t len, bool negative,
                           numeric_base_t base, size_t precision, size_t width,
                           printf_flags_t flags) {
    size_t unpadded_len = len;

    // pad with leading zeros
    if(!(flags & FLAGS_LEFT)) {
        if(width && (flags & FLAGS_ZEROPAD) &&
           (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
            width--;
        }
        while((flags & FLAGS_ZEROPAD) && (len < width) &&
              (len < PRINTF_INTEGER_BUFFER_SIZE)) {
            buf[len++] = '0';
        }
    }

    while((len < precision) && (len < PRINTF_INTEGER_BUFFER_SIZE)) {
        buf[len++] = '0';
    }

    if(base == BASE_OCTAL && (len > unpadded_len)) {
        // Since we've written some zeros, we've satisfied the alternative format
        // leading space requirement
        flags &= ~FLAGS_HASH;
    }

    // handle hash
    if(flags & (FLAGS_HASH | FLAGS_POINTER)) {
        if(!(flags & FLAGS_PRECISION) && len && ((len == precision) || (len == width))) {
            // Let's take back some padding digits to fit in what will eventually be
            // the format-specific prefix
            if(unpadded_len < len) {
                len--;  // This should suffice for BASE_OCTAL
            }
            if(len && (base == BASE_HEX || base == BASE_BINARY) && (unpadded_len < len)) {
                len--;  // ... and an extra one for 0x or 0b
            }
        }
        if((base == BASE_HEX) && !(flags & FLAGS_UPPERCASE) &&
           (len < PRINTF_INTEGER_BUFFER_SIZE)) {
            buf[len++] = 'x';
        } else if((base == BASE_HEX) && (flags & FLAGS_UPPERCASE) &&
                  (len < PRINTF_INTEGER_BUFFER_SIZE)) {
            buf[len++] = 'X';
        } else if((base == BASE_BINARY) && (len < PRINTF_INTEGER_BUFFER_SIZE)) {
            buf[len++] = 'b';
        }
        if(len < PRINTF_INTEGER_BUFFER_SIZE) {
            buf[len++] = '0';
        }
    }

    if(len < PRINTF_INTEGER_BUFFER_SIZE) {
        if(negative) {
            buf[len++] = '-';
        } else if(flags & FLAGS_PLUS) {
            buf[len++] = '+';  // ignore the space if the '+' exists
        } else if(flags & FLAGS_SPACE) {
            buf[len++] = ' ';
        }
    }

    out_rev_(output, buf, len, width, flags);
}

// An internal itoa-like function
static void
print_integer(output_t *output, printf_unsigned_value_t value, bool negative,
              numeric_base_t base, size_t precision, size_t width, printf_flags_t flags) {
    char buf[PRINTF_INTEGER_BUFFER_SIZE];
    size_t len = 0U;

    if(!value) {
        if(!(flags & FLAGS_PRECISION)) {
            buf[len++] = '0';
            flags &= ~FLAGS_HASH;
            // We drop this flag this since either the alternative and regular modes
            // of the specifier don't differ on 0 values, or (in the case of octal)
            // we've already provided the special handling for this mode.
        } else if(base == BASE_HEX) {
            flags &= ~FLAGS_HASH;
            // We drop this flag this since either the alternative and regular modes
            // of the specifier don't differ on 0 values
        }
    } else {
        do {
            const char digit = (char)(value % base);
            buf[len++] =
                (char)(digit < 10 ? '0' + digit
                                  : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10);
            value /= base;
        } while(value && (len < PRINTF_INTEGER_BUFFER_SIZE));
    }

    print_integer_finalization(output, buf, len, negative, base, precision, width, flags);
}

static void
print_floating_point(output_t *output, double value, size_t precision,
                     size_t width, printf_flags_t flags) {
    if((flags & FLAGS_PLUS) && value > 0.0)
        putchar_(output, '+');

    // set default precision, if not set explicitly
    //if(!(flags & FLAGS_PRECISION) || precision > PRINTF_DECIMAL_BUFFER_SIZE - 5)
    //    precision = PRINTF_DEFAULT_FLOAT_PRECISION;

    char buf[PRINTF_DECIMAL_BUFFER_SIZE];
    unsigned len = dtoa(value, buf); // Fill the buffer (TODO: Consider precision)
    out_(output, buf, len); // Print the buffer
}

// Advances the format pointer past the flags, and returns the parsed flags
// due to the characters passed
static printf_flags_t
parse_flags(const char **format) {
    printf_flags_t flags = 0U;
    do {
        switch(**format) {
        case '0': flags |= FLAGS_ZEROPAD; break;
        case '-': flags |= FLAGS_LEFT; break;
        case '+': flags |= FLAGS_PLUS; break;
        case ' ': flags |= FLAGS_SPACE; break;
        case '#': flags |= FLAGS_HASH; break;
        default: return flags;
        }
        (*format)++;
    } while(true);
}

#define ADVANCE_IN_FORMAT_STRING(cptr_)                                 \
    do {                                                                \
        (cptr_)++;                                                      \
        if(!*(cptr_))                                                   \
            return;                                                     \
    } while(0)

static void
format_string_loop(output_t *output, const char *format, va_list args) {
    while(*format) {
        if(*format != '%') {
            // A regular content character
            putchar_(output, *format);
            format++;
            continue;
        }
        // We're parsing a format specifier: %[flags][width][.precision][length]
        ADVANCE_IN_FORMAT_STRING(format);

        printf_flags_t flags = parse_flags(&format);

        // evaluate width field
        size_t width = 0U;
        if(is_digit_(*format)) {
            width = atou_(&format);
        } else if(*format == '*') {
            const int w = va_arg(args, int);
            if(w < 0) {
                flags |= FLAGS_LEFT;  // reverse padding
                width = (size_t)-w;
            } else {
                width = (size_t)w;
            }
            ADVANCE_IN_FORMAT_STRING(format);
        }

        // evaluate precision field
        size_t precision = 0U;
        if(*format == '.') {
            flags |= FLAGS_PRECISION;
            ADVANCE_IN_FORMAT_STRING(format);
            if(is_digit_(*format)) {
                precision = atou_(&format);
            } else if(*format == '*') {
                const int precision_ = va_arg(args, int);
                precision = precision_ > 0 ? (size_t)precision_ : 0U;
                ADVANCE_IN_FORMAT_STRING(format);
            }
        }

        // evaluate length field
        switch(*format) {
            case 'l':
                flags |= FLAGS_LONG;
                ADVANCE_IN_FORMAT_STRING(format);
                if(*format == 'l') {
                    flags |= FLAGS_LONG_LONG;
                    ADVANCE_IN_FORMAT_STRING(format);
                }
                break;
            case 'h':
                flags |= FLAGS_SHORT;
                ADVANCE_IN_FORMAT_STRING(format);
                if(*format == 'h') {
                    flags |= FLAGS_CHAR;
                    ADVANCE_IN_FORMAT_STRING(format);
                }
                break;
            case 't':
                flags |=
                    (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
                ADVANCE_IN_FORMAT_STRING(format);
                break;
            case 'j':
                flags |=
                    (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
                ADVANCE_IN_FORMAT_STRING(format);
                break;
            case 'z':
                flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
                ADVANCE_IN_FORMAT_STRING(format);
                break;
            default:
                break;
        }

        // evaluate specifier
        switch(*format) {
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
            case 'b': {
                if(*format == 'd' || *format == 'i') {
                    flags |= FLAGS_SIGNED;
                }

                numeric_base_t base;
                if(*format == 'x' || *format == 'X') {
                    base = BASE_HEX;
                } else if(*format == 'o') {
                    base = BASE_OCTAL;
                } else if(*format == 'b') {
                    base = BASE_BINARY;
                } else {
                    base = BASE_DECIMAL;
                    flags &=
                        ~FLAGS_HASH;  // decimal integers have no alternative presentation
                }

                if(*format == 'X') {
                    flags |= FLAGS_UPPERCASE;
                }

                format++;
                // ignore '0' flag when precision is given
                if(flags & FLAGS_PRECISION) {
                    flags &= ~FLAGS_ZEROPAD;
                }

                if(flags & FLAGS_SIGNED) {
                    // A signed specifier: d, i or possibly I + bit size if enabled
                    if(flags & FLAGS_LONG_LONG) {
                        const long long value = va_arg(args, long long);
                        print_integer(output, ABS_FOR_PRINTING(value), value < 0, base,
                                      precision, width, flags);
                    } else if(flags & FLAGS_LONG) {
                        const long value = va_arg(args, long);
                        print_integer(output, ABS_FOR_PRINTING(value), value < 0, base,
                                      precision, width, flags);
                    } else {
                        // We never try to interpret the argument as something
                        // potentially-smaller than int, due to integer promotion rules:
                        // Even if the user passed a short int, short unsigned etc. -
                        // these will come in after promotion, as int's (or unsigned for
                        // the case of short unsigned when it has the same size as int)
                        const int value =
                            (flags & FLAGS_CHAR)    ? (signed char)va_arg(args, int)
                            : (flags & FLAGS_SHORT) ? (short int)va_arg(args, int)
                                                    : va_arg(args, int);
                        print_integer(output, ABS_FOR_PRINTING(value), value < 0, base,
                                      precision, width, flags);
                    }
                } else {
                    // An unsigned specifier: u, x, X, o, b
                    flags &= ~(FLAGS_PLUS | FLAGS_SPACE);

                    if(flags & FLAGS_LONG_LONG) {
                        print_integer(output, (printf_unsigned_value_t)
                                      va_arg(args, unsigned long long),
                                      false, base, precision, width, flags);
                    } else if(flags & FLAGS_LONG) {
                        print_integer(output, (printf_unsigned_value_t)
                                      va_arg(args, unsigned long),
                                      false, base, precision, width, flags);
                    } else {
                        const unsigned int value = (flags & FLAGS_CHAR)
                            ? (unsigned char)va_arg(args, unsigned int)
                            : (flags & FLAGS_SHORT)
                            ? (unsigned short int)va_arg(args, unsigned int)
                            : va_arg(args, unsigned int);
                        print_integer(output, (printf_unsigned_value_t)value, false, base,
                                      precision, width, flags);
                    }
                }
                break;
            }

            case 'f':
            case 'F':
                if(*format == 'F')
                    flags |= FLAGS_UPPERCASE;
                print_floating_point(output, (double)va_arg(args, double),
                                     precision, width, flags);
                format++;
                break;

            case 'c': {
                size_t l = 1U;
                // pre padding
                if(!(flags & FLAGS_LEFT)) {
                    while(l++ < width) {
                        putchar_(output, ' ');
                    }
                }
                // char output
                putchar_(output, (char)va_arg(args, int));
                // post padding
                if(flags & FLAGS_LEFT) {
                    while(l++ < width) {
                        putchar_(output, ' ');
                    }
                }
                format++;
                break;
            }

            case 's': {
                const char *p = va_arg(args, char *);
                if(p == NULL) {
                    out_rev_(output, ")llun(", 6, width, flags);
                } else {
                    // string length
                    size_t l = strnlen_s_(p, precision ? precision : INT32_MAX);
                    if(flags & FLAGS_PRECISION) {
                        l = (l < precision ? l : precision);
                    }

                    // pre padding
                    if(!(flags & FLAGS_LEFT)) {
                        for(size_t i = 0; l + i < width; i++) {
                            putchar_(output, ' ');
                        }
                    }

                    // string output
                    out_(output, p, l);

                    // post padding
                    if(flags & FLAGS_LEFT) {
                        for(size_t i = 0; l + i < width; i++) {
                            putchar_(output, ' ');
                        }
                    }
                }
                format++;
                break;
            }

            case 'p': {
                width =
                    sizeof(void *) * 2U + 2;  // 2 hex chars per byte + the "0x" prefix
                flags |= FLAGS_ZEROPAD | FLAGS_POINTER;
                uintptr_t value = (uintptr_t)va_arg(args, void *);
                (value == (uintptr_t)NULL)
                    ? out_rev_(output, ")lin(", 5, width, flags)
                    : print_integer(output, (printf_unsigned_value_t)value, false,
                                    BASE_HEX, precision, width, flags);
                format++;
                break;
            }

            case '%':
                putchar_(output, '%');
                format++;
                break;

            default:
                putchar_(output, *format);
                format++;
                break;
        }
    }
}

int
mp_vsnprintf(char *s, size_t n, const char *format, va_list arg) {
    // Check that the inputs are sane
    if(!s || n < 1)
        return -1;

    // Format the string
    output_t out = {s, 0, n};
    format_string_loop(&out, format, arg);

    // Write the string-terminating '\0' character
    size_t null_char_pos = out.pos < out.max_chars ? out.pos : out.max_chars - 1;
    out.buffer[null_char_pos] = '\0';

    // Return written chars without terminating \0
    return (int)out.pos;
}

int
mp_snprintf(char *s, size_t n, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int ret = mp_vsnprintf(s, n, format, args);
    va_end(args);
    return ret;
}
