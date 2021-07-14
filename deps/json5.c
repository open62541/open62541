#include "json5.h"

#include <malloc.h>
#include <assert.h>

#ifndef JSON5_ASSERT
#define JSON5_ASSERT assert(0)
#endif

static char *json5__parse_object(json5_object *obj, char *base, json5_error *err_code);
static char *json5__parse_value(json5_object *obj, char *base, json5_error *err_code);

static void
json5_init(json5_object *obj, json5_type type) {
    memset(obj, 0, sizeof(json5_object));
    obj->type = type;
}

static json5_error
json5_append(json5_object *obj, json5_object *elem) {
    if(obj->type != json5_type_array && obj->type != json5_type_object)
        return json5_error_internal;
    json5_object *newArr = (json5_object*)
        realloc(obj->content.elements, sizeof(json5_object) * (obj->size + 1));
    if(!newArr)
        return json5_error_no_memory;
    obj->content.elements = newArr;
    obj->content.elements[obj->size] = *elem;
    obj->size += 1;
    return json5_error_none;
}

void
json5_clear(json5_object *obj) {
    if(obj->type != json5_type_array && obj->type != json5_type_object)
        return;
    if(!obj->content.elements)
        return;

    for(size_t i = 0; i < obj->size; ++i)
        json5_clear(&obj->content.elements[i]);

    free(obj->content.elements);
}

inline bool
json5__is_control_char(char c) {
    return !!strchr("\"\\/bfnrt", c);
}

inline char *
json5__trim(char *str) {
    while (*str && isspace(*str)) {
        ++str;
    }
    return str;
}

inline char *
json5__skip(char *str, char c) {
    while ((*str && *str != c) || (*(str - 1) == '\\' &&
                                   *str == c && json5__is_control_char(c))) {
        ++str;
    }
    return str;
}

inline bool
json5__validate_name(char *s, char *err) {
    while (*s) {
        if ((s[0] == '\\' && !json5__is_control_char(s[1])) &&
            (s[0] == '\\' && !isxdigit(s[1]) && !isxdigit(s[2]) &&
             !isxdigit(s[3]) && !isxdigit(s[4]))) {
            *err = *s;
            return false;
        }
        ++s;
    }
    return true;
}

static void
json5__strip_comments(char *dest) {
    bool is_lit = false;
    char lit_c = '\0';
    char *p = dest;
    char *b = dest;
    size_t l = 0;
    
    while (*p) {
        if(!is_lit) {
            if((*p == '"' || *p == '\'')) {
                lit_c = *p;
                is_lit = true;
                ++p;
                continue;
            }
        } else {
            if(*p == '\\' && *(p + 1) && *(p + 1) == lit_c) {
                p += 2;
                continue;
            } else if (*p == lit_c) {
                is_lit = false;
                ++p;
                continue;
            }
        }
        
        if(!is_lit) {
            if(p[0] == '/' && p[1] == '*') {
                b = p;
                l = 2;
                p += 2;
                
                while(p[0] != '*' && p[1] != '/') {
                    ++p; ++l;
                }
                p += 2;
                l += 2;
                memset(b, ' ', l);
            }
            if (p[0] == '/' && p[1] == '/' ) { // @rlyeh, was: p[0] == '/' && p[0] == '/'
                b = p;
                l = 2;
                p += 2;
                
                while (p[0] != '\n') {
                    ++p;
                    ++l;
                }
                ++p;
                ++l;
                memset(b, ' ', l);
            }
        }
        
        ++p;
    }
}

static char *
json5__parse_array(json5_object *obj, char *p, json5_error *err_code) {
    assert(obj && p);

    json5_init(obj, json5_type_array);

    while(*p) {
        json5_object elem;
        json5_init(&elem, json5_type_object);

        p = json5__trim(p);
        p = json5__parse_value(&elem, p, err_code);
        if(*err_code != json5_error_none)
            return NULL;

        json5_append(obj, &elem);

        p = json5__trim(p);
        if(*p != ',')
            return p;
        ++p;
    }
    return p;
}

char *
json5__parse_value(json5_object *obj, char *base, json5_error *err_code) {
    assert(obj && base);
    char *p = base;
    char *b = base;
    char *e = base;

    if(*p == '[') {
        /* Parse an array */
        p = json5__trim(p + 1);
        if(*p == ']') return p;
        p = json5__parse_array(obj, p, err_code);
        if(*err_code != json5_error_none)
            return NULL;
        ++p;
    } else if(*p == '{') {
        /* Parse an object */
        p = json5__trim(p + 1);
        p = json5__parse_object(obj, p, err_code);
        if(*err_code != json5_error_none)
            return NULL;
        ++p;
    } else if(*p == '"' || *p == '\'') {
        /* Parse a string */
        char c = *p;
        obj->type = json5_type_string;
        b = p + 1;
        e = b;
        obj->content.string = b;
        while(*e) {
            if(*e == '\\' && *(e + 1) == c) {
                e += 2;
                continue;
            }
            if(*e == '\\' && (*(e + 1) == '\r' || *(e + 1) == '\n')) {
                *e = ' ';
                e++;
                continue;
            }
            if(*e == c)
                break;
            ++e;
        }
        *e = '\0';
        p = e + 1;
    } else if(*p == '`') {
        /* Parse a multistring */
        obj->type = json5_type_multistring;
        b = p + 1;
        e = b;
        obj->content.string = b;
        while (*e) {
            if(*e == '\\' && *(e + 1) == '`') {
                e += 2;
                continue;
            }
            if(*e == '`')
                break;
            ++e;
        }
        *e = '\0';
        p = e + 1;
    } else if (isalpha(*p) || (*p == '-' && !isdigit(p[1]))) {
        /* Parse a special value */
        if (!strncmp(p, "true", 4)) {
            p += 4;
            obj->type = json5_type_true;
        }
        else if (!strncmp(p, "false", 5)) {
            p += 5;
            obj->type = json5_type_false;
        }
        else if (!strncmp(p, "null", 4)) {
            p += 4;
            obj->type = json5_type_null;
        }
        else if (!strncmp(p, "Infinity", 8)) {
            p += 8;
            obj->type = json5_type_real;
            obj->content.real = INFINITY;
        }
        else if (!strncmp(p, "-Infinity", 9)) {
            p += 9;
            obj->type = json5_type_real;
            obj->content.real = -INFINITY;
        }
        else if (!strncmp(p, "NaN", 3)) {
            p += 3;
            obj->type = json5_type_real;
            obj->content.real = NAN;
        }
        else if (!strncmp(p, "-NaN", 4)) {
            p += 4;
            obj->type = json5_type_real;
            obj->content.real = -NAN;
        }
        else {
            *err_code = json5_error_invalid_value;
            return NULL;
        }
    } else if (isdigit(*p) || *p == '+' || *p == '-' || *p == '.') {
        /* Parse a number (integer or real) */
        obj->type = json5_type_integer;

        b = p;
        e = b;

        int ib = 0;
        char buf[16] = { 0 };

        if(*e == '+')
            ++e;
        else if (*e == '-') {
            buf[ib++] = *e++;
        }

        if (*e == '.') {
            obj->type = json5_type_real;
            buf[ib++] = '0';

            do {
                buf[ib++] = *e;
            } while (isdigit(*++e));
        } else {
            while (isxdigit(*e) || *e == 'x' || *e == 'X') {
                buf[ib++] = *e++;
            }

            if (*e == '.') {
                obj->type = json5_type_real;
                uint32_t step = 0;

                do {
                    buf[ib++] = *e;
                    ++step;
                } while (isdigit(*++e));

                if (step < 2) {
                    buf[ib++] = '0';
                }
            }
        }

        int64_t exp = 0; float eb = 10;
        char expbuf[6] = { 0 };
        int expi = 0;

        if(*e == 'e' || *e == 'E') {
            ++e;
            if (*e == '+' || *e == '-' || isdigit(*e)) {
                if (*e == '-') {
                    eb = 0.1f;
                }

                if (!isdigit(*e)) {
                    ++e;
                }

                while (isdigit(*e)) {
                    expbuf[expi++] = *e++;
                }

            }
            exp = strtol(expbuf, NULL, 10);
        }

        if(*e == '\0') {
            *err_code = json5_error_invalid_value;
            return NULL;
        }

        if(obj->type == json5_type_integer) {
            obj->content.integer = strtol(buf, 0, 0);
            while (--exp > 0) {
                obj->content.integer *= (int64_t)eb;
            }
        } else {
            obj->content.real = atof(buf);
            while (--exp > 0) {
                obj->content.real *= eb;
            }
        }
        p = e;
    }

    return p;
}

char *
json5__parse_object(json5_object *obj, char *base, json5_error *err_code) {
    assert(obj && base);
    char *p = base;
    char *b = base;
    char *e = base;

    json5_init(obj, json5_type_object);

    p = json5__trim(p);

    /* Objects may begin with { but don't have to. E.g. the outermost object. */
    bool braced = false;
    if(*p == '{') {
        braced = true;
        p++;
    }

    while(*p) {
        p = json5__trim(p);

        /* The object can finish with a }, but only if it started with one. */
        if(braced && *p == '}')
            return p;

        json5_object node = { 0 };

        if(*p == '"' || *p == '\'') {
            /* Handle a quoted name. Extract the name and set \0 at its end. */
            char c = *p;
            b = ++p;
            e = json5__skip(b, c);
            *e = '\0';
            node.name = b;

            /* Find the colon before the value */
            p = ++e;
            p = json5__trim(p);
            if(*p && *p != ':') {
                *err_code = json5_error_invalid_name;
                return NULL;
            }
        } else {
            if(*p == '[') {
                node.name = 0; // @rlyeh, was: *node.name = '\0';
                p = json5__parse_value(&node, p, err_code);
                goto l_parsed;
            }
            else if(isalpha(*p) || *p == '_' || *p == '$') {
                b = p;
                e = b;

                do {
                    ++e;
                } while (*e &&
                         (*e == '_' || isalpha(*e) || isdigit(*e)) &&
                         !isspace(*e) && *e != ':');

                if (*e == ':') {
                    p = e;
                } else {
                    while (*e) {
                        if (*e && (!isspace(*e) || *e == ':')) {
                            break;
                        }
                        ++e;
                    }
                    e = json5__trim(e);
                    p = e;

                    if(*p && *p != ':') {
                        *err_code = json5_error_invalid_name;
                        return NULL;
                    }
                }

                *e = '\0';
                node.name = b;
            }
        }

        char errc;
        if(!json5__validate_name(node.name, &errc)) {
            *err_code = json5_error_invalid_name;
            return NULL;
        }

        p = json5__trim(p + 1);
        p = json5__parse_value(&node, p, err_code);
        if(*err_code != json5_error_none)
            return NULL;

    l_parsed:
        /* Append the parsed node */
        json5_append(obj, &node);

        p = json5__trim(p);

        /* Continue to the next node */
        if(*p == '\0' || *p == '}')
            continue;
        if(*p == ',') {
            ++p;
        } else {
            *err_code = json5_error_invalid_value;
            return NULL;
        }
    }
    return p;
}

json5_error
json5_parse(json5_object *root, char *source) {
    assert(root && source);

    json5__strip_comments(source);

    json5_error err_code = json5_error_none;
    json5_object root_ = { 0 };
    json5__parse_object(&root_, source, &err_code);

    *root = root_;
    return err_code;
}
