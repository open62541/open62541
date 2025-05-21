#ifndef NANO_FILE_H
#define NANO_FILE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nng/nng.h"

NNG_DECL bool    nano_file_exists(const char *fpath);
NNG_DECL char *  nano_getcwd(char *buf, size_t size);
NNG_DECL int64_t nano_getline(
    char **restrict line, size_t *restrict len, FILE *restrict fp);
NNG_DECL char * nano_concat_path(const char *dir, const char *file_name);
NNG_DECL int    file_write_string(const char *fpath, const char *string);
NNG_DECL int    file_create_dir(const char *fpath);
NNG_DECL size_t file_load_data(const char *filepath, void **data);

#endif
