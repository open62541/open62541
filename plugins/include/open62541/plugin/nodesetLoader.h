#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <open62541/types.h>
struct UA_Server;
typedef UA_UInt16 (*addNamespaceCb)(struct UA_Server *server, const char *);

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    struct UA_Server *server;
} FileHandler;

struct Nodeset;
UA_StatusCode
UA_XmlImport_loadFile(const FileHandler *fileHandler);
#endif
