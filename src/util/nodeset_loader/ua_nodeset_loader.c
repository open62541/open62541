/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2021 (c) Jan Murzyn
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server.h>

#include "ua_nodeset_loader_internal.h"
#include <string.h>

#include "yxml.h"

typedef enum { XML_TOKENIZE_OK, XML_TOKENIZE_INVALID, XML_TOKENIZE_OVERFLOW } XmlTokenizeStatus;

typedef struct {
    XmlTokenizeStatus status;
    size_t tokensSize;
} XmlTokenizeResult;

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} XmlTextBuffer;

#define XML_TOKEN_STACK_SIZE 128
#define XML_YXML_STACK_SIZE 4096

static bool
xmlTextBufferAppend(XmlTextBuffer *buf, const char *data, size_t length) {
    if(buf->size > buf->capacity || length > buf->capacity - buf->size)
        return false;
    memcpy(buf->data + buf->size, data, length);
    buf->size += length;
    return true;
}

static bool
xmlTextBufferAppendName(XmlTextBuffer *buf, const char *name, size_t length, char **result) {
    *result = buf->data + buf->size;
    if(!xmlTextBufferAppend(buf, name, length) || !xmlTextBufferAppend(buf, "", 1))
        return false;
    char *colon = strrchr(*result, ':');
    if(colon)
        *result = colon + 1;
    return true;
}

static XmlTokenizeResult
xmlTokenize(const char *xml, size_t xmlLength, XmlToken *tokens, size_t maxTokens,
            XmlTextBuffer *text) {
    XmlTokenizeResult result;
    memset(&result, 0, sizeof(result));
    size_t tokenPosition = 0;

    text->size = 0;
    if(!xmlTextBufferAppend(text, "", 1))
        goto invalid;

    yxml_t parser;
    char parserStack[XML_YXML_STACK_SIZE];
    yxml_init(&parser, parserStack, sizeof(parserStack));

    XmlToken *stack[XML_TOKEN_STACK_SIZE];
    bool hasChildren[XML_TOKEN_STACK_SIZE];
    memset(stack, 0, sizeof(stack));
    memset(hasChildren, 0, sizeof(hasChildren));

    XmlToken *attribute = NULL;
    size_t depth = 0;

    for(size_t pos = 0; pos < xmlLength; pos++) {
        yxml_ret_t status = yxml_parse(&parser, (unsigned char)xml[pos]);
        if(status < YXML_OK)
            goto invalid;

        switch(status) {
        case YXML_OK:
        case YXML_PISTART:
        case YXML_PICONTENT:
        case YXML_PIEND:
            break;

        case YXML_ELEMSTART: {
            if(depth >= XML_TOKEN_STACK_SIZE)
                goto invalid;
            if(depth > 0) {
                hasChildren[depth - 1] = true;
                if(stack[depth - 1])
                    stack[depth - 1]->content = NULL;
            }

            XmlToken *token = tokenPosition < maxTokens ? &tokens[tokenPosition] : NULL;
            size_t nameLength = yxml_symlen(&parser, parser.elem);
            if(token) {
                memset(token, 0, sizeof(*token));
                if(!xmlTextBufferAppendName(text, parser.elem, nameLength, &token->name))
                    goto invalid;
                if(nameLength < pos + 1)
                    token->start = pos - nameLength - 1;
            }
            stack[depth] = token;
            hasChildren[depth] = false;
            depth++;
            tokenPosition++;
            break;
        }

        case YXML_ATTRSTART: {
            if(depth == 0)
                goto invalid;
            if(stack[depth - 1])
                stack[depth - 1]->attributes++;
            attribute = tokenPosition < maxTokens ? &tokens[tokenPosition] : NULL;
            if(attribute) {
                memset(attribute, 0, sizeof(*attribute));
                attribute->content = text->data; /* Empty string sentinel */
                size_t nameLength = yxml_symlen(&parser, parser.attr);
                if(!xmlTextBufferAppendName(text, parser.attr, nameLength, &attribute->name))
                    goto invalid;
            }
            tokenPosition++;
            break;
        }

        case YXML_CONTENT:
            if(depth > 0 && stack[depth - 1] && !hasChildren[depth - 1]) {
                size_t length = strlen(parser.data);
                if(!stack[depth - 1]->content)
                    stack[depth - 1]->content = text->data + text->size;
                if(!xmlTextBufferAppend(text, parser.data, length))
                    goto invalid;
            }
            break;

        case YXML_ATTRVAL:
            if(attribute) {
                size_t length = strlen(parser.data);
                if(attribute->content == text->data)
                    attribute->content = text->data + text->size;
                if(!xmlTextBufferAppend(text, parser.data, length))
                    goto invalid;
            }
            break;

        case YXML_ATTREND:
            if(attribute && attribute->content != text->data && !xmlTextBufferAppend(text, "", 1))
                goto invalid;
            attribute = NULL;
            break;

        case YXML_ELEMEND:
            if(depth == 0)
                goto invalid;
            depth--;
            if(stack[depth]) {
                stack[depth]->end = pos + 1;
                stack[depth]->subtreeEnd = tokenPosition;
                if(stack[depth]->content && !xmlTextBufferAppend(text, "", 1))
                    goto invalid;
            }
            break;

        default:
            goto invalid;
        }
    }

    result.tokensSize = tokenPosition;
    if(yxml_eof(&parser) != YXML_OK || depth != 0)
        goto invalid;
    if(tokenPosition > maxTokens)
        result.status = XML_TOKENIZE_OVERFLOW;
    return result;

invalid:
    result.status = XML_TOKENIZE_INVALID;
    result.tokensSize = tokenPosition;
    return result;
}

typedef enum {
    XML_SCOPE_DOCUMENT,
    XML_SCOPE_NODE,
    XML_SCOPE_NAMESPACE_URIS,
    XML_SCOPE_REFERENCES,
    XML_SCOPE_DEFINITION
} XmlScope;

typedef struct {
    const XmlToken *tokens;
    size_t tokensSize;
    size_t position;
    const char *xml;
    size_t xmlLength;
} XmlCursor;

static bool
xmlTokenNodeClass(const char *name, UA_NodeClass *nodeClass) {
    static const struct {
        const char *name;
        UA_NodeClass nodeClass;
    } nodeClasses[] = {{"UAObject", UA_NODECLASS_OBJECT},
                       {"UAObjectType", UA_NODECLASS_OBJECTTYPE},
                       {"UAVariable", UA_NODECLASS_VARIABLE},
                       {"UADataType", UA_NODECLASS_DATATYPE},
                       {"UAMethod", UA_NODECLASS_METHOD},
                       {"UAReferenceType", UA_NODECLASS_REFERENCETYPE},
                       {"UAVariableType", UA_NODECLASS_VARIABLETYPE},
                       {"UAView", UA_NODECLASS_VIEW}};
    for(size_t i = 0; i < sizeof(nodeClasses) / sizeof(nodeClasses[0]); i++) {
        if(strcmp(name, nodeClasses[i].name))
            continue;
        *nodeClass = nodeClasses[i].nodeClass;
        return true;
    }
    return false;
}

static char *
xmlTokenLeafContent(XmlCursor *cursor, const XmlToken *element) {
    cursor->position = element->subtreeEnd;
    if(!element->content)
        return NULL;
    return element->content;
}

static bool processElement(NodeSet *nodeset, XmlCursor *cursor, XmlScope scope, NL_Node *node);

static bool
processChildren(NodeSet *nodeset, XmlCursor *cursor, const XmlToken *element, XmlScope scope,
                NL_Node *node) {
    if(element->subtreeEnd < cursor->position || element->subtreeEnd > cursor->tokensSize)
        return false;
    while(cursor->position < element->subtreeEnd) {
        if(!processElement(nodeset, cursor, scope, node))
            return false;
    }
    return cursor->position == element->subtreeEnd;
}

static bool
processElement(NodeSet *nodeset, XmlCursor *cursor, XmlScope scope, NL_Node *node) {
    if(cursor->position >= cursor->tokensSize)
        return false;

    const XmlToken *element = &cursor->tokens[cursor->position++];
    if(element->subtreeEnd < cursor->position || element->subtreeEnd > cursor->tokensSize)
        return false;

    size_t attributePosition = cursor->position;
    if(element->attributes > element->subtreeEnd - cursor->position)
        return false;
    cursor->position += element->attributes;

    const char *name = element->name;
    XmlAttributes attributes = {&cursor->tokens[attributePosition], element->attributes};

    if(scope == XML_SCOPE_DOCUMENT) {
        UA_NodeClass nodeClass;
        if(xmlTokenNodeClass(name, &nodeClass)) {
            NL_Node *newNode = UA_NodeSet_newNode(nodeset, nodeClass, &attributes);
            if(!newNode)
                return false;
            return processChildren(nodeset, cursor, element, XML_SCOPE_NODE, newNode);
        } else if(!strcmp(name, "NamespaceUris")) {
            return processChildren(nodeset, cursor, element, XML_SCOPE_NAMESPACE_URIS, NULL);
        } else if(!strcmp(name, "Alias")) {
            char *content = xmlTokenLeafContent(cursor, element);
            return UA_NodeSet_addAlias(nodeset, &attributes, content);
        } else if(!strcmp(name, "UANodeSet") || !strcmp(name, "Aliases")) {
            return processChildren(nodeset, cursor, element, XML_SCOPE_DOCUMENT, NULL);
        }
    } else if(scope == XML_SCOPE_NODE) {
        if(!strcmp(name, "DisplayName")) {
            char *content = xmlTokenLeafContent(cursor, element);
            UA_NodeSet_setLocalizedText(&node->displayName, &attributes, content);
            return true;
        } else if(!strcmp(name, "References")) {
            return processChildren(nodeset, cursor, element, XML_SCOPE_REFERENCES, node);
        } else if(!strcmp(name, "Description")) {
            char *content = xmlTokenLeafContent(cursor, element);
            UA_NodeSet_setLocalizedText(&node->description, &attributes, content);
            return true;
        } else if(!strcmp(name, "Value")) {
            cursor->position = element->subtreeEnd;
            if(node->nodeClass != UA_NODECLASS_VARIABLE)
                return true;
            if(element->end < element->start || element->end > cursor->xmlLength)
                return false;
            UA_String xmlValue = {element->end - element->start,
                                  (UA_Byte *)(uintptr_t)(cursor->xml + element->start)};
            return UA_String_copy(&xmlValue, &((NL_VariableNode *)node)->value) ==
                   UA_STATUSCODE_GOOD;
        } else if(!strcmp(name, "Definition") && node->nodeClass == UA_NODECLASS_DATATYPE) {
            if(!UA_NodeSet_addDataTypeDefinition(node, &attributes))
                return false;
            return processChildren(nodeset, cursor, element, XML_SCOPE_DEFINITION, node);
        } else if(!strcmp(name, "InverseName")) {
            char *content = xmlTokenLeafContent(cursor, element);
            if(node->nodeClass == UA_NODECLASS_REFERENCETYPE)
                UA_NodeSet_setLocalizedText(&((NL_ReferenceTypeNode *)node)->inverseName,
                                            &attributes, content);
            return true;
        }
    } else if(scope == XML_SCOPE_NAMESPACE_URIS) {
        if(!strcmp(name, "Uri")) {
            char *content = xmlTokenLeafContent(cursor, element);
            return content &&
                   UA_NodeSet_addNamespace(nodeset, UA_STRING(content)) == UA_STATUSCODE_GOOD;
        }
    } else if(scope == XML_SCOPE_REFERENCES) {
        if(!strcmp(name, "Reference")) {
            char *content = xmlTokenLeafContent(cursor, element);
            return UA_NodeSet_addReference(nodeset, node, &attributes, content);
        }
    } else if(scope == XML_SCOPE_DEFINITION) {
        if(!strcmp(name, "Field")) {
            if(!UA_NodeSet_addDataTypeField(nodeset, node, &attributes))
                return false;
            cursor->position = element->subtreeEnd;
            return true;
        }
    }

    /* Unknown elements are irrelevant together with their entire subtree. */
    cursor->position = element->subtreeEnd;
    return true;
}

UA_StatusCode
UA_NodeSet_import(NodeSet *nodeset, const UA_XmlElement *xml) {
    if(!nodeset || nodeset->text || !xml || (!xml->data && xml->length > 0) ||
       xml->length == SIZE_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    XmlToken tokenBuffer[64];
    XmlToken *tokens = tokenBuffer;
    size_t tokensCapacity = 64;
    /* Decoded names and leaf values cannot exceed their source XML size. */
    XmlTextBuffer text = {(char *)UA_malloc(xml->length + 1), 0, xml->length + 1};
    if(!text.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    XmlTokenizeResult result =
        xmlTokenize((const char *)xml->data, xml->length, tokens, tokensCapacity, &text);
    if(result.status == XML_TOKENIZE_OVERFLOW) {
        tokensCapacity = result.tokensSize;
        tokens = NULL;
        if(tokensCapacity <= SIZE_MAX / sizeof(XmlToken))
            tokens = (XmlToken *)UA_malloc(tokensCapacity * sizeof(XmlToken));
        if(tokens)
            result =
                xmlTokenize((const char *)xml->data, xml->length, tokens, tokensCapacity, &text);
    }

    UA_StatusCode status = UA_STATUSCODE_BADDECODINGERROR;
    if(tokens && result.status == XML_TOKENIZE_OK && result.tokensSize > 0) {
        XmlCursor cursor = {tokens, result.tokensSize, 0, (const char *)xml->data, xml->length};
        nodeset->text = text.data;
        bool processed = processElement(nodeset, &cursor, XML_SCOPE_DOCUMENT, NULL);
        if(processed && cursor.position == cursor.tokensSize)
            status = UA_STATUSCODE_GOOD;
        else
            UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                         "NodeSetLoader: Invalid XML structure at token %u of %u",
                         (unsigned)cursor.position, (unsigned)cursor.tokensSize);
    } else if(!tokens) {
        status = UA_STATUSCODE_BADOUTOFMEMORY;
    } else {
        UA_LOG_ERROR(nodeset->logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: XML tokenization failed");
    }

    if(tokens != tokenBuffer)
        UA_free(tokens);
    if(nodeset->text != text.data)
        UA_free(text.data);
    return status;
}

UA_StatusCode
UA_Server_loadNodeset(UA_Server *server, const UA_XmlElement nodesetXml) {
    if(!server || (!nodesetXml.data && nodesetXml.length > 0))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Logger *logger = config->logging;

    NodeSet nodeset;
    UA_StatusCode res = UA_NodeSet_init(&nodeset, server);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: Could not initialize import (%s)", UA_StatusCode_name(res));
        return res;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SERVER, "NodeSetLoader: Start importing nodeset");
    res = UA_NodeSet_import(&nodeset, &nodesetXml);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeSet_apply(&nodeset);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "NodeSetLoader: Importing the nodeset failed (%s)", UA_StatusCode_name(res));
    UA_NodeSet_clear(&nodeset);
    return res;
}
