/*
 * UA_list.h
 *
 *  Created on: Mar 13, 2014
 *      Author: sten
 */

#ifndef UA_LIST_H_
#define UA_LIST_H_

#include "opcua_builtInDatatypes.h"

/*
 * Data Structures
 */
typedef void (*UA_list_PayloadVisitor)(void* payload);

/*
 * Double Linked Lists
 */

typedef struct T_UA_list_Element {
	struct T_UA_list_List* father;
	void *payload;
    struct T_UA_list_Element* next;
    struct T_UA_list_Element* prev;
}UA_list_Element;

typedef struct T_UA_list_List {
   struct T_UA_list_Element* first;
   struct T_UA_list_Element* last;
   Int32 size;
}UA_list_List;

typedef void (*UA_list_ElementVisitor)(UA_list_Element* payload);
/*
 * Returns 1 for true, 0 otherwise
 */
typedef Boolean (*UA_list_PayloadMatcher)(void* payload);

Int32 UA_list_initElement(UA_list_Element* const element);

Int32 UA_list_init(UA_list_List* const list);

Int32 UA_list_addElementToFront(UA_list_List* const list, UA_list_Element* const element);

Int32 UA_list_addPayloadToFront(UA_list_List* const list, void* const payload);

Int32 UA_list_addElementToBack(UA_list_List* const list, UA_list_Element* const element);

Int32 UA_list_addPayloadToBack(UA_list_List* const list, void* const payload);

Int32 UA_list_removeFirst(UA_list_List* const list, UA_list_PayloadVisitor visitor);

Int32 UA_list_removeLast(UA_list_List* const list, UA_list_PayloadVisitor visitor);

Int32 UA_list_removeElement(UA_list_Element* const elem, UA_list_PayloadVisitor visitor);

Int32 UA_list_destroy(UA_list_List* const list, UA_list_PayloadVisitor visitor);

Int32 UA_list_iterateElement(UA_list_List* const list, UA_list_ElementVisitor visitor);

Int32 UA_list_iteratePayload(UA_list_List* const list, UA_list_PayloadVisitor visitor);

UA_list_Element* UA_list_find(UA_list_List* const list, UA_list_PayloadMatcher matcher);

UA_list_Element* UA_list_getFirst(UA_list_List* const list);

UA_list_Element* UA_list_getLast(UA_list_List* const list);

#endif /* UA_LIST_H_ */
