%module open62541
%{
/* Includes the header in the wrapper code */
#include "open62541_expanded.h"
%}

/* Parse the header file to generate wrappers */
%include "stdint.i"
%include "open62541_expanded.h"
