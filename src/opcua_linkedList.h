/*
 * opcua_linkedList.h
 *
 *  Created on: Feb 5, 2014
 *      Author: opcua
 */

#ifndef OPCUA_LINKEDLIST_H_
#define OPCUA_LINKEDLIST_H_

#include "opcua_advancedDatatypes.h";
#include "opcua_types.h"
#include "opcua_builtInDatatypes.h";

typedef struct T_element
{
   AD_RawMessage *binaryData;
   int(*serviceImplementation)(AD_RawMessage *data, AD_RawMessage *response);
   struct T_linkedList * next;
};

typedef struct T_element element;

#endif /* OPCUA_LINKEDLIST_H_ */

