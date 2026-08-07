/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_FILESTORE_COMMON_H_
#define UA_FILESTORE_COMMON_H_

#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) || defined(__APPLE__)

#ifdef UA_ARCHITECTURE_WIN32
#include <direct.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "tr_dirent.h"

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

#define UA_SEEK_END SEEK_END
#define UA_SEEK_SET SEEK_SET
#define UA_DT_REG DT_REG
#define UA_DT_DIR DT_DIR
#define UA_PATH_MAX MAX_PATH
#define UA_FILENAME_MAX FILENAME_MAX

_UA_BEGIN_DECLS
char *
_UA_dirname_minimal(char *path);
_UA_END_DECLS
#define UA_dirname _UA_dirname_minimal
#else
#include "../../arch/posix/eventloop_posix.h"
#endif

UA_StatusCode
readFileToByteString(const char *const path,
                     UA_ByteString *data);

UA_StatusCode
writeByteStringToFile(const char *const path,
                      const UA_ByteString *data);

#endif /* defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) || defined(__APPLE__) */

#endif /* UA_ENABLE_ENCRYPTION */

#endif /* UA_FILESTORE_COMMON_H_ */
