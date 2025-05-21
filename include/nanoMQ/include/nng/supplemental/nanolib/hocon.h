#ifndef HOCON_H
#define HOCON_H
#include "cJSON.h"

cJSON *hocon_parse_file(const char *file);
cJSON *hocon_parse_str(char *str, size_t len);


#endif