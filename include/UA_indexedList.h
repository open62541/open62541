#ifndef UA_INDEXEDLIST_H_
#define UA_INDEXEDLIST_H_

#include "opcua_builtInDatatypes.h"
/* UA_indexedList reuses many types of UA_list */
#include "UA_list.h"

/*
 * Integer Indexed List
 */
typedef struct T_UA_indexedList_Element {
	struct T_UA_list_Element* father;
	Int32 index;
	void* payload;
}UA_indexedList_Element;

typedef UA_list_List UA_indexedList_List;
typedef UA_list_PayloadVisitor UA_indexedList_PayloadVisitor;

void UA_indexedList_defaultFreer(void* payload);

Int32 UA_indexedList_init(UA_indexedList_List* const list);

Int32 UA_indexedList_destroy(UA_indexedList_List* const list, UA_indexedList_PayloadVisitor visitor);

Int32 UA_indexedList_initElement(UA_indexedList_Element* const elem);

Int32 UA_indexedList_addValue(UA_indexedList_List* const list, Int32 index, void* payload);

Int32 UA_indexedList_addValueToFront(UA_indexedList_List* const list, Int32 index, void* payload);

UA_indexedList_Element* UA_indexedList_find(UA_indexedList_List* const list, Int32 index);

void* UA_indexedList_findValue(UA_indexedList_List* const list, Int32 index);

Int32 UA_indexedList_iterateValues(UA_indexedList_List* const list, UA_indexedList_PayloadVisitor visitor);

Int32 UA_indexedList_removeElement(UA_indexedList_List* const list, UA_indexedList_Element* elem, UA_indexedList_PayloadVisitor visitor);

#endif /* UA_INDEXEDLIST_H_ */
