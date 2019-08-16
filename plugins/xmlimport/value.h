
#ifndef VALUE_H
#define VALUE_H
#include <open62541/plugin/nodestore.h>

struct Value;
typedef struct Value Value;

struct Value *
Value_new(const UA_Node* node);

void
Value_start(Value *val, const UA_Node *node, const char* localname);

void
Value_end(Value *val, UA_Node *node, const char* localname, char *value);

void
Value_finish(Value *val, UA_Node *node);

#endif
