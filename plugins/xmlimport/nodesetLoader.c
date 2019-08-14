/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 */

#include <open62541/plugin/nodesetLoader.h>
#include <open62541/plugin/nodestore.h>

#include <stdio.h>
#include <string.h>

#include "nodeset.h"
#include "value.h"
#include <libxml/SAX.h>

#define OBJECT "UAObject"
#define METHOD "UAMethod"
#define OBJECTTYPE "UAObjectType"
#define VARIABLE "UAVariable"
#define VARIABLETYPE "UAVariableType"
#define DATATYPE "UADataType"
#define REFERENCETYPE "UAReferenceType"
#define DISPLAYNAME "DisplayName"
#define REFERENCES "References"
#define REFERENCE "Reference"
#define DESCRIPTION "Description"
#define ALIAS "Alias"
#define NAMESPACEURIS "NamespaceUris"
#define NAMESPACEURI "Uri"
#define VALUE "Value"

typedef enum {
    PARSER_STATE_INIT,
    PARSER_STATE_NODE,
    PARSER_STATE_DISPLAYNAME,
    PARSER_STATE_REFERENCES,
    PARSER_STATE_REFERENCE,
    PARSER_STATE_DESCRIPTION,
    PARSER_STATE_ALIAS,
    PARSER_STATE_UNKNOWN,
    PARSER_STATE_NAMESPACEURIS,
    PARSER_STATE_URI,
    PARSER_STATE_VALUE
} TParserState;

typedef struct {
    TParserState state;
    TParserState prev_state;
    size_t unknown_depth;
    UA_NodeClass nodeClass;
    UA_Node *node;
    UA_NodeReferenceKind *refKind;
    Alias *alias;
    void *userContext;
    char *onCharacters;
    Nodeset *nodeset;
    struct Value *val;
} TParserCtx;

static void enterUnknownState(TParserCtx *ctx) {
    ctx->prev_state = ctx->state;
    ctx->state = PARSER_STATE_UNKNOWN;
    ctx->unknown_depth = 1;
}

static void
OnStartElementNs(void *ctx, const char *localname, const char *prefix, const char *URI,
                 int nb_namespaces, const char **namespaces, int nb_attributes,
                 int nb_defaulted, const char **attributes) {
    TParserCtx *pctx = (TParserCtx *)ctx;
    switch(pctx->state) {
        case PARSER_STATE_INIT:
            if(!strcmp(localname, VARIABLE)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_VARIABLE;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, OBJECT)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_OBJECT;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, OBJECTTYPE)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_OBJECTTYPE;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, DATATYPE)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_DATATYPE;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, METHOD)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_METHOD;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, REFERENCETYPE)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_REFERENCETYPE;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, VARIABLETYPE)) {
                pctx->state = PARSER_STATE_NODE;
                pctx->nodeClass = UA_NODECLASS_VARIABLETYPE;
                pctx->node = Nodeset_newNode(pctx->nodeset, pctx->nodeClass,
                                             nb_attributes, attributes);
                pctx->state = PARSER_STATE_NODE;
            } else if(!strcmp(localname, NAMESPACEURIS)) {
                pctx->state = PARSER_STATE_NAMESPACEURIS;
            } else if(!strcmp(localname, ALIAS)) {
                pctx->state = PARSER_STATE_ALIAS;
                pctx->node = NULL;
                Alias *alias = Nodeset_newAlias(pctx->nodeset, nb_attributes, attributes);
                pctx->alias = alias;
                pctx->state = PARSER_STATE_ALIAS;
                pctx->onCharacters = NULL;
            } else if(!strcmp(localname, "UANodeSet") || !strcmp(localname, "Aliases") ||
                      !strcmp(localname, "Extensions")) {
                pctx->state = PARSER_STATE_INIT;
            } else {
                enterUnknownState(pctx);
            }
            break;
        case PARSER_STATE_NAMESPACEURIS:
            if(!strcmp(localname, NAMESPACEURI)) {
                Nodeset_newNamespace(pctx->nodeset);
                pctx->state = PARSER_STATE_URI;
            } else {
                enterUnknownState(pctx);
            }
            break;
        case PARSER_STATE_URI:
            enterUnknownState(pctx);
            break;
        case PARSER_STATE_NODE:
            if(!strcmp(localname, DISPLAYNAME)) {
                pctx->state = PARSER_STATE_DISPLAYNAME;
            } else if(!strcmp(localname, REFERENCES)) {
                pctx->state = PARSER_STATE_REFERENCES;
            } else if(!strcmp(localname, DESCRIPTION)) {
                pctx->state = PARSER_STATE_DESCRIPTION;
            }else if(!strcmp(localname, VALUE)) {
                pctx->val = Value_new();
                pctx->state = PARSER_STATE_VALUE;
            } else {
                enterUnknownState(pctx);
            }
            break;
        case PARSER_STATE_VALUE:
            Value_start(pctx->val, pctx->node, localname);
            break;

        case PARSER_STATE_REFERENCES:
            if(!strcmp(localname, REFERENCE)) {
                pctx->state = PARSER_STATE_REFERENCE;
                pctx->refKind = Nodeset_newReference(pctx->nodeset, pctx->node,
                                                     nb_attributes, attributes);
            } else {
                enterUnknownState(pctx);
            }
            break;
        case PARSER_STATE_DESCRIPTION:
            enterUnknownState(pctx);
            break;
        case PARSER_STATE_ALIAS:
            enterUnknownState(pctx);
            break;
        case PARSER_STATE_DISPLAYNAME:
            enterUnknownState(pctx);
            break;
        case PARSER_STATE_REFERENCE:
            enterUnknownState(pctx);
            break;
        case PARSER_STATE_UNKNOWN:
            pctx->unknown_depth++;
            break;
    }
    pctx->onCharacters = NULL;
}

static void OnEndElementNs(void *ctx, const char *localname, const char *prefix,
                           const char *URI) {
    TParserCtx *pctx = (TParserCtx *)ctx;
    switch(pctx->state) {
        case PARSER_STATE_INIT:
            break;
        case PARSER_STATE_ALIAS:
            Nodeset_newAliasFinish(pctx->nodeset, pctx->alias, pctx->onCharacters);
            pctx->onCharacters = NULL;
            pctx->state = PARSER_STATE_INIT;
            break;
        case PARSER_STATE_URI: {
            Nodeset_newNamespaceFinish(pctx->nodeset, pctx->userContext,
                                       pctx->onCharacters);
            pctx->state = PARSER_STATE_NAMESPACEURIS;
        } break;
        case PARSER_STATE_NAMESPACEURIS:
            pctx->state = PARSER_STATE_INIT;
            break;
        case PARSER_STATE_NODE:
            Nodeset_newNodeFinish(pctx->nodeset, pctx->node);
            pctx->state = PARSER_STATE_INIT;
            break;
        case PARSER_STATE_DESCRIPTION:
            pctx->state = PARSER_STATE_NODE;
            break;
        case PARSER_STATE_DISPLAYNAME:
            Nodeset_setDisplayname(pctx->node, pctx->onCharacters);
            pctx->state = PARSER_STATE_NODE;
            break;
        case PARSER_STATE_REFERENCES:
            pctx->state = PARSER_STATE_NODE;
            break;
        case PARSER_STATE_REFERENCE:
            Nodeset_newReferenceFinish(pctx->nodeset, pctx->refKind, pctx->onCharacters);
            pctx->state = PARSER_STATE_REFERENCES;
            break;
        case PARSER_STATE_VALUE:            
            if(!strcmp(localname, "Value"))
            {
                Value_finish(pctx->val, pctx->node);
                pctx->state = PARSER_STATE_NODE;
            }else
            {
                Value_end(pctx->val, pctx->node, localname, pctx->onCharacters);
            }            
            break;
        case PARSER_STATE_UNKNOWN:
            pctx->unknown_depth--;
            if(pctx->unknown_depth == 0) {
                pctx->state = pctx->prev_state;
            }
    }
    pctx->onCharacters = NULL;
}

static void
OnCharacters(void *ctx, const char *ch, int len) {

    TParserCtx *pctx = (TParserCtx *)ctx;
    char *oldString = pctx->onCharacters;
    size_t oldLength = 0;
    if(oldString != NULL) {
        oldLength = strlen(oldString);
    }
    char *newValue = (char *)UA_malloc(oldLength + (size_t)len + 1);
    if(oldString != NULL) {
        memcpy(newValue, oldString, oldLength);
    }
    memcpy(newValue + oldLength, ch, (size_t)len);
    Nodeset_addRefCountedChar(pctx->nodeset, newValue);
    newValue[oldLength + (size_t)len] = '\0';
    pctx->onCharacters = newValue;
}

static xmlSAXHandler make_sax_handler(void) {
    xmlSAXHandler SAXHandler;
    memset(&SAXHandler, 0, sizeof(xmlSAXHandler));
    SAXHandler.initialized = XML_SAX2_MAGIC;
    // nodesets are encoded with UTF-8
    // this code does no transformation on the encoded text or interprets it
    // so it should be safe to cast xmlChar* to char*
    SAXHandler.startElementNs = (startElementNsSAX2Func)OnStartElementNs;
    SAXHandler.endElementNs = (endElementNsSAX2Func)OnEndElementNs;
    SAXHandler.characters = (charactersSAXFunc)OnCharacters;
    return SAXHandler;
}

static int read_xmlfile(FILE *f, TParserCtx *parserCtxt) {
    char chars[1024];
    int res = (int)fread(chars, 1, 4, f);
    if(res <= 0) {
        return 1;
    }

    xmlSAXHandler SAXHander = make_sax_handler();
    xmlParserCtxtPtr ctxt =
        xmlCreatePushParserCtxt(&SAXHander, parserCtxt, chars, res, NULL);
    while((res = (int)fread(chars, 1, sizeof(chars), f)) > 0) {
        if(xmlParseChunk(ctxt, chars, res, 0)) {
            xmlParserError(ctxt, "xmlParseChunk");
            return 1;
        }
    }
    xmlParseChunk(ctxt, chars, 0, 1);
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    return 0;
}

UA_StatusCode UA_XmlImport_loadFile(const FileHandler *fileHandler) {

    if(fileHandler == NULL) {
        printf("no filehandler - return\n");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(fileHandler->addNamespace == NULL) {
        printf("no fileHandler->addNamespace - return\n");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(fileHandler->file == NULL) {
        printf("no fileHandler->file return\n");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode status = UA_STATUSCODE_GOOD;

    TParserCtx *ctx = (TParserCtx *)malloc(sizeof(TParserCtx));
    ctx->state = PARSER_STATE_INIT;
    ctx->prev_state = PARSER_STATE_INIT;
    ctx->unknown_depth = 0;
    ctx->onCharacters = NULL;
    ctx->userContext = NULL;
    ctx->nodeset = Nodeset_new(fileHandler->server);

    Nodeset_setNewNamespaceCallback(ctx->nodeset, fileHandler->addNamespace);

    FILE *f = fopen(fileHandler->file, "r");
    if(!f) {
        puts("file open error.");
        status = UA_STATUSCODE_BADNOTFOUND;
        goto cleanup;
    }

    if(read_xmlfile(f, ctx)) {
        puts("xml read error.");
        status = UA_STATUSCODE_BADNOTFOUND;
    }

    Nodeset_linkReferences(ctx->nodeset);

cleanup:
    Nodeset_cleanup(ctx->nodeset);
    UA_free(ctx);
    if(f) {
        fclose(f);
    }
    return status;
}
