/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license.
 */

#ifndef STRING_ESCAPE_H
#define STRING_ESCAPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <open62541/architecture_definitions.h>
#include <string.h>
    
/* Locale independent versions of isxxx() functions */
#define l_isupper(c)  ('A' <= (c) && (c) <= 'Z')
#define l_islower(c)  ('a' <= (c) && (c) <= 'z')
#define l_isalpha(c)  (l_isupper(c) || l_islower(c))
#define l_isdigit(c)  ('0' <= (c) && (c) <= '9')
#define l_isxdigit(c) (l_isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))

int utf8_encode(int32_t codepoint, char *buffer, size_t *size);
size_t utf8_check_first(char byte);
size_t utf8_check_full(const char *buffer, size_t size, int32_t *codepoint);
const char *utf8_iterate(const char *buffer, size_t size, int32_t *codepoint);
int utf8_check_string(const char *string, size_t length);
int32_t decode_unicode_escape(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* STRING_ESCAPE_H */

