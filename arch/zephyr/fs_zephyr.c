#include <zephyr/fs/fs.h>

extern int UA_fileExists(const char *path)
{
    struct fs_dirent entry;
    int rc = fs_stat(path, &entry);
    return rc == 0;          // true if found, false if not
}