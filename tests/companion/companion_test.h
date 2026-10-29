/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMPANION_TEST_H_
#define COMPANION_TEST_H_
#include <open62541/server.h>

typedef struct {
    const char *typeUri;
    const char *typeIdentifier;
    const char *targetUri;
    const char *targetIdentifier;
    UA_Boolean forward;
} CompanionReference;

typedef struct {
    const char *uri;
    const char *identifier;
    UA_UInt32 attributes;
    const CompanionReference *references;
    size_t referencesSize;
} CompanionNode;
typedef struct {
    size_t index;
    const char *path;
    const size_t *dependencies;
    size_t dependenciesSize;
    const CompanionNode *nodes;
    size_t nodesSize;
    UA_Boolean preferred;
    UA_StatusCode (*load)(UA_Server *, UA_Boolean);
    size_t typesSize;
    const UA_DataType *(*staticType)(size_t);
} CompanionModule;
extern const CompanionModule *const companionModules[];
extern const size_t companionModulesSize;
int companionRun(size_t selected, UA_Boolean copyTypes);
#endif
