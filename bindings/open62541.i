/** building for lua
 swig -lua -I../include open62541.i
 gcc -fpic -c open62541_wrap.c -I/usr/include/lua5.2 -I../include -I../build/src_generated
 gcc -shared open62541_wrap.o ../build/libopen62541.a -llua5.2 -o open62541.so
*/

%module open62541
%{
#include "ua_types.h"
#include "ua_server.h"
%}

%define UA_TYPE_HANDLING_FUNCTIONS_SWIG(TYPE)
    TYPE * TYPE##_new(void);
    void TYPE##_init(TYPE * p);
    void TYPE##_delete(TYPE * p);
    void TYPE##_deleteMembers(TYPE * p);
    UA_StatusCode TYPE##_copy(const TYPE *src, TYPE *dst);
%enddef

%define UA_TYPE_HANDLING_FUNCTIONS_AS_SWIG(TYPE)
    TYPE * TYPE##_new(void);
    void TYPE##_init(TYPE * p);
    void TYPE##_delete(TYPE * p);
    void TYPE##_deleteMembers(TYPE * p);
    UA_StatusCode TYPE##_copy(const TYPE *src, TYPE *dst);
%enddef

%define UA_EXPORT
%enddef

UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Boolean)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_SByte)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Byte)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Int16)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_UInt16)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Int32)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_UInt32)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Int64)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_UInt64)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Float)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Double)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_String)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_DateTime)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Guid)
UA_TYPE_HANDLING_FUNCTIONS_AS_SWIG(UA_ByteString)
UA_TYPE_HANDLING_FUNCTIONS_AS_SWIG(UA_XmlElement)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_NodeId)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_ExpandedNodeId)
UA_TYPE_HANDLING_FUNCTIONS_AS_SWIG(UA_StatusCode)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_QualifiedName)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_LocalizedText)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_ExtensionObject)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_DataValue)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_Variant)
UA_TYPE_HANDLING_FUNCTIONS_SWIG(UA_DiagnosticInfo)

%include "ua_types.h"
%include "ua_server.h"
