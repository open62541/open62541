/*
 swig -python -I../src -I../examples/src open62541.i
 gcc -c open62541_wrap.c -I/usr/include/python2.7 -I../examples/src -I../src -fPIC
 ld -shared open62541_wrap.o ../lib/libopen62541.so ../examples/src/networklayer.so -o _open62541.so
*/

%module open62541
%{
/* Includes the header in the wrapper code */
#include "networklayer.h"
#include "ua_application.h"
%}

/* Parse the header file to generate wrappers */
typedef int UA_Int32;
//%include "stdint.i"
//%include "ua_types.h"
%include "networklayer.h"
%include "ua_application.h"

extern NL_Description NL_Description_TcpBinary;
extern Application appMockup;
void appMockup_init();
