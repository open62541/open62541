#ifndef NODESETLOADER_H
#define NODESETLOADER_H
#include <stdbool.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_xml.h>

struct Nodeset;
bool
loadXmlFile(struct Nodeset *nodeset, const FileHandler *fileHandler);

#endif