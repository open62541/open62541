#include "../common/fileSystemOperations_common.h"

#if defined(UA_ARCHITECTURE_WIN32)
#include <direct.h>
#include <stdio.h>

/*Create a directory with the give path*/
UA_StatusCode
makeDirectory(const char *path) {
    if(_mkdir(path) == 0)
        return UA_STATUSCODE_GOOD;
    else
        return UA_STATUSCODE_BADINTERNALERROR;
}

/*Create a File with the give path*/
UA_StatusCode
makeFile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    fclose(file);
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_WIN32