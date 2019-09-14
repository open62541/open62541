#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <open62541/types.h>
#include <open62541/server_config.h>

_UA_BEGIN_DECLS
typedef UA_UInt16 (*addNamespaceCb)(UA_Server *server, const char *);

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    UA_Server *server;
} FileHandler;

struct Nodeset;

UA_StatusCode
UA_XmlImport_loadFile(const FileHandler *fileHandler);
_UA_END_DECLS
#endif
