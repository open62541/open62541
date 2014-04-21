/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct parent {
	int depth;
	void* obj[20];
} parent_t;

void startElement(void * data, const char *el, const char **attr) {
  parent_t* p = (parent_t*) data;
  int i, j;


  if (p->depth==0) { printf("new %s --\n", el); }
  p->obj[p->depth] = el;
  for (i = 0; attr[i]; i += 2) {
	  for (j = 0; j < p->depth; j++) {
		  printf("%s.",p->obj[j]);
	  }
	  printf("%s='%s'\n", attr[i], attr[i + 1]);
  }
  p->depth++;
}  /* End of start handler */

void handleText(void * data, const char *s, int len) {
  parent_t* p = (parent_t*) data;
  int j, i;

  if (len > 0) {
	  // process only strings that are not entirely built of whitespaces
	  for (i=0; i<len;i++) {
		  if (! isspace(s[i])) {
			  for (j = 0; j < p->depth; j++) {
				  printf("%s.",p->obj[j]);
			  }
			  printf("%s={%d,'%.*s'}\n", "Value", len, len, s);
			  break;
		  }
	  }
  }
}  /* End of text handler */

void endElement(void *data, const char *el) {
	parent_t* p = (parent_t*) data;
	p->depth--;
}  /* End of end handler */

int main()
{
  char buf[1024];
  int len;   /* len is the number of bytes in the current bufferful of data */
  int done;
  parent_t p;
  p.depth = 0;

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
