/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION) && defined(UA_ENABLE_CERTIFICATE_FILESTORE)

#if defined(UA_ARCHITECTURE_WIN32)

#include <direct.h>
#include <minwindef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "dirent.h"

_UA_BEGIN_DECLS
char *
_UA_dirname_minimal(char *path);
_UA_END_DECLS

#define UA_STAT stat
#define UA_DIR DIR
#define UA_DIRENT dirent
#define UA_FILE FILE
#define UA_MODE uint16_t

#define UA_stat stat
#define UA_opendir opendir
#define UA_readdir readdir
#define UA_rewinddir rewinddir
#define UA_closedir closedir
#define UA_mkdir(path, mode) _mkdir(path)
#define UA_fopen fopen
#define UA_fread fread
#define UA_fwrite fwrite
#define UA_fseek fseek
#define UA_ftell ftell
#define UA_fclose fclose
#define UA_remove remove
#define UA_dirname _UA_dirname_minimal

#define UA_SEEK_END SEEK_END
#define UA_SEEK_SET SEEK_SET
#define UA_DT_REG DT_REG
#define UA_DT_DIR DT_DIR
#define UA_PATH_MAX MAX_PATH
#define UA_FILENAME_MAX FILENAME_MAX

#elif defined(UA_ARCHITECTURE_POSIX)

#ifdef __linux__

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef __ANDROID__
#include <bits/stdio_lim.h>
#endif /* !__ANDROID__ */

#define UA_STAT stat
#define UA_DIR DIR
#define UA_DIRENT dirent
#define UA_FILE FILE
#define UA_MODE mode_t

#define UA_stat stat
#define UA_opendir opendir
#define UA_readdir readdir
#define UA_rewinddir rewinddir
#define UA_closedir closedir
#define UA_mkdir mkdir
#define UA_fopen fopen
#define UA_fread fread
#define UA_fwrite fwrite
#define UA_fseek fseek
#define UA_ftell ftell
#define UA_fclose fclose
#define UA_remove remove
#define UA_dirname dirname

#define UA_SEEK_END SEEK_END
#define UA_SEEK_SET SEEK_SET
#define UA_DT_REG DT_REG
#define UA_DT_DIR DT_DIR
#define UA_PATH_MAX PATH_MAX
#define UA_FILENAME_MAX FILENAME_MAX

#endif /* __linux__ */

#endif

UA_StatusCode
readFileToByteString(const char *const path,
                     UA_ByteString *data);

UA_StatusCode
writeByteStringToFile(const char *const path,
                      const UA_ByteString *data);

#endif /* UA_ENABLE_ENCRYPTION && UA_ENABLE_CERTIFICATE_FILESTORE */
