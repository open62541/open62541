#include "ua_namespace_xml.h"
#include <fcntl.h> // open, O_RDONLY

typedef UA_Int32 (*XML_Stack_Loader) (char* buf, int len);

#define XML_BUFFER_LEN 1024
UA_Int32 Namespace_loadXml(Namespace **ns,UA_UInt32 nsid,const char* rootName, XML_Stack_Loader getNextBufferFull) {
	UA_Int32 retval = UA_SUCCESS;
	char buf[XML_BUFFER_LEN];
	int len; /* len is the number of bytes in the current bufferful of data */

	XML_Stack s;
	XML_Stack_init(&s, 0, rootName);

	UA_NodeSet n;
	UA_NodeSet_init(&n, 0);
	*ns = n.ns;

	XML_Stack_addChildHandler(&s, "UANodeSet", strlen("UANodeSet"), (XML_decoder) UA_NodeSet_decodeXmlFromStack, UA_INVALIDTYPE, &n);
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &s);
	XML_SetElementHandler(parser, XML_Stack_startElement, XML_Stack_endElement);
	XML_SetCharacterDataHandler(parser, XML_Stack_handleText);
	while ((len = getNextBufferFull(buf, XML_BUFFER_LEN)) > 0) {
		if (XML_Parse(parser, buf, len, (len < XML_BUFFER_LEN)) == XML_STATUS_ERROR) {
			retval = UA_ERR_INVALID_VALUE;
			break;
		}
	}
	XML_ParserFree(parser);

	DBG_VERBOSE(printf("Namespace_loadXml - aliases addr=%p, size=%d\n", (void*) &(n.aliases), n.aliases.size));
	DBG_VERBOSE(UA_NodeSetAliases_println("Namespace_loadXml - elements=", &n.aliases));

	return retval;
}

static int theFile = 0;
UA_Int32 readFromTheFile(char*buf,int len) {
	return read(theFile,buf,len);
}

/** @brief load a namespace from an XML-File
 *
 * @param[in/out] ns the address of the namespace ptr
 * @param[in] namespaceId the numeric id of the namespace
 * @param[in] rootName the name of the root element of the hierarchy (not used?)
 * @param[in] fileName the name of an existing file, e.g. Opc.Ua.NodeSet2.xml
 */
UA_Int32 Namespace_loadFromFile(Namespace **ns,UA_UInt32 nsid,const char* rootName,const char* fileName) {
	if (fileName == UA_NULL)
		theFile = 0; // stdin
	else if ((theFile = open(fileName, O_RDONLY)) == -1)
		return UA_ERR_INVALID_VALUE;

	UA_Int32 retval = Namespace_loadXml(ns,nsid,rootName,readFromTheFile);
	close(theFile);
	return retval;
}

static const char* theBuffer = UA_NULL;
static const char* theBufferEnd = UA_NULL;
UA_Int32 readFromTheBuffer(char*buf,int len) {
	if (len == 0) return 0;
	if (theBuffer + XML_BUFFER_LEN > theBufferEnd)
		len = theBufferEnd - theBuffer + 1;
	else
		len = XML_BUFFER_LEN;
	memcpy(buf,theBuffer,len);
	theBuffer = theBuffer + len;
	return len;
}

/** @brief load a namespace from a string
 *
 * @param[in/out] ns the address of the namespace ptr
 * @param[in] namespaceId the numeric id of the namespace
 * @param[in] rootName the name of the root element of the hierarchy (not used?)
 * @param[in] buffer the xml string
 */
UA_Int32 Namespace_loadFromString(Namespace **ns,UA_UInt32 nsid,const char* rootName,const char* buffer) {
	theBuffer = buffer;
	theBufferEnd = buffer + strlen(buffer) - 1;
	return Namespace_loadXml(ns,nsid,rootName,readFromTheBuffer);
}
