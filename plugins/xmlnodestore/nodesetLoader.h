#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <stdbool.h>
#include <open62541/plugin/nodestore.h>

#define NODECLASS_COUNT 7
typedef enum {
    NODECLASS_OBJECT = 0,
    NODECLASS_OBJECTTYPE = 1,
    NODECLASS_VARIABLE = 2,
    NODECLASS_DATATYPE = 3,
    NODECLASS_METHOD = 4,
    NODECLASS_REFERENCETYPE = 5,
    NODECLASS_VARIABLETYPE = 6
    // TODO: eventtype missing
} TNodeClass;

const UA_NodeClass UA_NODECLASSES[NODECLASS_COUNT] = {
    UA_NODECLASS_OBJECT,      UA_NODECLASS_OBJECTTYPE, UA_NODECLASS_VARIABLE,
    UA_NODECLASS_DATATYPE,    UA_NODECLASS_METHOD,     UA_NODECLASS_REFERENCETYPE,
    UA_NODECLASS_VARIABLETYPE};

typedef UA_UInt16 (*addNamespaceCb)(void* userContext, const char *);

typedef struct {
    int loadTimeMs;
    int sortTimeMs;
    int addNodeTimeMs;
} Statistics;

typedef struct {
    const char *file;
    addNamespaceCb addNamespace;
    const Statistics *stat;
    void *userContext;
} FileHandler;

bool loadFile(const FileHandler *fileHandler);

#endif