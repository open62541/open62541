/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen

#include "opcua.h"


typedef char const * const XML_Attr_t;
typedef char const * cstring_t;
struct XML_Stack;
typedef UA_Int32 (*XML_decoder)(struct XML_Stack* s, XML_Attr_t* attr, void* dst);

typedef struct XML_child {
	cstring_t name;
	UA_Int32 type;
	XML_decoder handler;
	void* obj;
} XML_child_t;

typedef struct XML_Parent {
	cstring_t name;
	int textAttribIdx; // -1 - not set
	cstring_t textAttrib;
	int len; // -1 - empty set
	XML_child_t children[20];
} XML_Parent_t;

typedef struct XML_Stack {
	int depth;
	XML_Parent_t parent[10];
} XML_Stack_t;

void XML_Stack_init(XML_Stack_t* p, cstring_t name) {
	unsigned int i,j;
	p->depth = 0;
	for (i=0;i<10;i++) {
		p->parent[i].name = UA_NULL;
		p->parent[i].len = 0;
		p->parent[i].textAttrib = UA_NULL;
		p->parent[i].textAttribIdx = -1;
		for (j=0;j<20;j++) {
			p->parent[i].children[j].name = UA_NULL;
			p->parent[i].children[j].handler = UA_NULL;
			p->parent[i].children[j].type = UA_INVALIDTYPE;
			p->parent[i].children[j].obj = UA_NULL;
		}
	}
	p->parent[0].name = name;
}

// FIXME: we might want to calculate textAttribIdx
void XML_Stack_handleTextAs(XML_Stack_t* p,cstring_t textAttrib, unsigned int textAttribIdx) {
	p->parent[p->depth].textAttrib = textAttrib;
	p->parent[p->depth].textAttribIdx = textAttribIdx;
}

void XML_Stack_addChildHandler(XML_Stack_t* p,cstring_t name,XML_decoder handler, UA_Int32 type, void* dst) {
	unsigned int len = p->parent[p->depth].len;
	p->parent[p->depth].children[len].name = name;
	p->parent[p->depth].children[len].handler = handler;
	p->parent[p->depth].children[len].type = type;
	p->parent[p->depth].children[len].obj = dst;
	p->parent[p->depth].len++;
}


UA_Int32 UA_NodeId_copycstring(cstring_t src,UA_NodeId* dst) {
	dst->encodingByte = UA_NODEIDTYPE_FOURBYTE;
	dst->namespace = 0;
	// FIXME: assumes i=nnnn, does not care for aliases as of now
	dst->identifier.numeric = atoi(&src[2]);
	return UA_SUCCESS;
}

typedef struct T_UA_NodeSet { int dummy; } UA_NodeSet;

UA_Int32 UA_Array_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, void* dst) {
	UA_UInt32 i;
	return UA_SUCCESS;
}

UA_Int32 UA_Int32_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_Int32* dst) {
	UA_UInt32 i;
	if (dst == UA_NULL) { UA_[UA_INT32].new((void**) &dst); }
	return UA_SUCCESS;
}

UA_Int32 UA_String_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_String* dst) {
	UA_UInt32 i;
	if (dst == UA_NULL) { UA_[UA_STRING].new((void**) &dst); }
	if (s->parent[s->depth].len == 0 ) {
		XML_Stack_addChildHandler(s,"Data",(XML_decoder)UA_Array_decodeXML, UA_BYTE, &(dst->data));
		XML_Stack_addChildHandler(s,"Length",(XML_decoder)UA_Int32_decodeXML, UA_INT32, &(dst->length));
		XML_Stack_handleTextAs(s,"Data",0);
	}
	// set attributes
	for (i = 0; attr[i]; i += 2) {
		if (0==strncmp("Data",attr[i],strlen("Data"))) {
			UA_String_copycstring(attr[i+1],dst);
		} else {
			perror("Unknown attribute");
		}
	}
	return UA_SUCCESS;
}

UA_Int32 UA_LocalizedText_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_LocalizedText* dst) {
	UA_UInt32 i;
	// create a new object if called with UA_NULL
	if (dst == UA_NULL) { UA_[UA_LOCALIZEDTEXT].new((void**) &dst); }
	if (s->parent[s->depth].len == 0 ) {
		XML_Stack_addChildHandler(s,"Text",(XML_decoder)UA_String_decodeXML, UA_STRING, &(dst->text));
		XML_Stack_addChildHandler(s,"Locale",(XML_decoder)UA_String_decodeXML, UA_STRING, &(dst->locale));
		XML_Stack_handleTextAs(s,"Data",0);
	}
	return UA_SUCCESS;
}

UA_Int32 UA_DataTypeNode_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_DataTypeNode* dst) {
	UA_UInt32 i;

	// create a new object if called with UA_NULL
	if (dst == UA_NULL) { UA_[UA_DATATYPENODE].new((void**) &dst); }

	// add the handlers for the child objects
	if (s->parent[s->depth].len == 0 ) {
		XML_Stack_addChildHandler(s,"DisplayName",(XML_decoder)UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT, &(dst->displayName));
		XML_Stack_addChildHandler(s,"Description",(XML_decoder)UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT, &(dst->description));
	}

	// set missing but implicit attributes
	dst->nodeClass = UA_NODECLASS_DATATYPE;

	// set attributes
	for (i = 0; attr[i]; i += 2) {
		if (0==strncmp("NodeId",attr[i],strlen("NodeId"))) {
			UA_NodeId_copycstring(attr[i+1],&(dst->nodeId));
		} else if (0==strncmp("BrowseName",attr[i],strlen("BrowseName"))) {
			UA_String_copycstring(attr[i+1],&(dst->browseName.name));
			dst->browseName.namespaceIndex = 0;
//		} else if (0==strncmp("DisplayName",attr[i],strlen("DisplayName"))) {
//			UA_String_copycstring(attr[i+1],&(dst->displayName.text));
//			dst->displayName.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
//		} else if (0==strncmp("Description",attr[i],strlen("Description"))) {
//			UA_String_copycstring(attr[i+1],&(dst->description.text));
//			dst->description.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
		} else {
			perror("Unknown attribute");
		}
	}
	return UA_SUCCESS;
}


UA_Int32 UA_NodeSet_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_NodeSet* dst) {
	if (s->parent[s->depth].len == 0 ) {
		XML_Stack_addChildHandler(s,"UADataType",(XML_decoder)UA_DataTypeNode_decodeXML, UA_DATATYPENODE, UA_NULL);
	}
	return UA_SUCCESS;
}

void startElement(void * data, const char *el, const char **attr) {
  XML_Stack_t* p = (XML_Stack_t*) data;
  int i, j;

  // scan expected children
  XML_Parent_t* cp = &p->parent[p->depth];
  for (i = 0; i < cp->len; i++) {
	  if (0 == strncmp(cp->children[i].name,el,strlen(cp->children[i].name))) {
		  printf("processing child ");
		  for (j=0;j<=p->depth;j++) {
			  printf("%s.",p->parent[j].name);
		  }
		  printf("%s\n",el);

		  p->depth++;
		  p->parent[p->depth].name = el;
		  p->parent[p->depth].len = 0;
		  p->parent[p->depth].textAttribIdx = -1;

		  // finally call the handler and return
		  cp->children[i].handler(data,attr,cp->children[i].obj);
		  return;
	  }
  }
  // if we come here we rejected the processing of el
  printf("rejected processing of unexpected child ");
  for (i=0;i<=p->depth;i++) {
	  printf("%s.",p->parent[i].name);
  }
  printf("%s\n",el);
  p->depth++;
  p->parent[p->depth].name = el;
  // this should be sufficient to reject the children as well
  p->parent[p->depth].len = 0;
}  /* End of start handler */

UA_Int32 XML_isSpace(cstring_t s, int len) {
	UA_Int32 retval = UA_TRUE;
	int i;
	for (i=0; i<len; i++) {
		if (! isspace(s[i])) {
		  return UA_FALSE;
		}
	}
	return UA_TRUE;
}

void handleText(void * data, const char *s, int len) {
  XML_Stack_t* p = (XML_Stack_t*) data;
  int j, i;

  if (len > 0 && ! XML_isSpace(s,len)) {
	  XML_Parent_t* cp = &(p->parent[p->depth]);
	  if (cp->textAttribIdx >= 0) {
		  int childIdx = cp->textAttribIdx;
		  char* buf; // need to copy s to add a 0 as terminator to string
		  UA_alloc((void**)&buf,len+1);
		  strncpy(buf,s,len);
		  buf[len+1] = 0;
		  XML_Attr_t attr[3] = { cp->textAttrib, buf, UA_NULL };
		  cp->children[childIdx].handler(p,attr,cp->children[childIdx].obj);
		  UA_free(buf);
	  }
  }
}  /* End of text handler */

void endElement(void *data, const char *el) {
	XML_Stack_t* p = (XML_Stack_t*) data;
	p->depth--;
}  /* End of end handler */

int main()
{
  char buf[1024];
  int len;   /* len is the number of bytes in the current bufferful of data */
  int done;
  XML_Stack_t p;
  XML_Stack_init(&p, "ROOT");
  XML_Stack_addChildHandler(&p,"UANodeSet", (XML_decoder) UA_NodeSet_decodeXML, UA_INVALIDTYPE, UA_NULL);

  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, &p);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, handleText);
  while ((len = read(0,buf,1024)) > 0) {
    if (!XML_Parse(parser, buf, len, (len<1024))) {
      return 1;
    }
  }
  XML_ParserFree(parser);
  return 0;
}
