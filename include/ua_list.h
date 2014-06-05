#ifndef UA_LIST_H_
#define UA_LIST_H_

#include "opcua.h"
#include <stddef.h> /* Needed for sys/queue.h */
#include <sys/queue.h>


/**********************/
/* Singly Linked List */
/**********************/

#define UA_SLIST_HEAD(name, type) SLIST_HEAD(name, type)
#define UA_SLIST_HEAD_INITIALIZER(head) SLIST_HEAD_INITILIZER(head)
#define UA_SLIST_ENTRY(type) SLIST_ENTRY(type)
#define UA_SLIST_INIT(head) SLIST_INIT(head)
#define UA_SLIST_INSERT_AFTER(slistelm, elm, field) SLIST_INSERT_AFTER(slistelm, elm, field)
#define UA_SLIST_INSERT_HEAD(head, elm, field) SLIST_INSERT_HEAD(head, elm, field)
#define UA_SLIST_REMOVE_HEAD(head, field) SLIST_REMOVE_HEAD(head, field)
#define UA_SLIST_REMOVE(head, elm, type, field) SLIST_REMOVE(head, elm, type, field)
#define UA_SLIST_FOREACH(var, head, field) SLIST_FOREACH(var, head, field)
#define UA_SLIST_EMPTY(head) SLIST_EMPTY(head)
#define UA_SLIST_FIRST(head) SLIST_FIRST(head)
#define UA_SLIST_NEXT(elm, field) SLIST_NEXT(elm, field)

/**********************/
/* Doubly Linked List */
/**********************/

typedef void (*UA_list_PayloadVisitor)(void* payload);

typedef struct UA_list_Element {
	struct UA_list_List* father;
	void *payload;
    struct UA_list_Element* next;
    struct UA_list_Element* prev;
}UA_list_Element;

typedef struct UA_list_List {
   struct UA_list_Element* first;
   struct UA_list_Element* last;
   UA_Int32 size;
}UA_list_List;

typedef void (*UA_list_ElementVisitor)(UA_list_Element* payload);

//typedef Boolean (*UA_list_PayloadMatcher)(void* payload);
typedef _Bool (*UA_list_PayloadMatcher)(void* payload);
typedef _Bool (*UA_list_PayloadComparer)(void* payload, void* otherPayload);

void UA_list_defaultFreer(void* payload);

UA_Int32 UA_list_initElement(UA_list_Element* element);

UA_Int32 UA_list_init(UA_list_List* list);

UA_Int32 UA_list_addElementToFront(UA_list_List* list, UA_list_Element* element);

UA_Int32 UA_list_addPayloadToFront(UA_list_List* list, void* const payload);

UA_Int32 UA_list_addElementToBack(UA_list_List* list, UA_list_Element* element);

UA_Int32 UA_list_addPayloadToBack(UA_list_List* list, void* const payload);

UA_Int32 UA_list_removeFirst(UA_list_List* list, UA_list_PayloadVisitor visitor);

UA_Int32 UA_list_removeLast(UA_list_List* list, UA_list_PayloadVisitor visitor);

UA_Int32 UA_list_removeElement(UA_list_Element* const elem, UA_list_PayloadVisitor visitor);

UA_Int32 UA_list_destroy(UA_list_List* list, UA_list_PayloadVisitor visitor);

UA_Int32 UA_list_iterateElement(UA_list_List* const list, UA_list_ElementVisitor visitor);

UA_Int32 UA_list_iteratePayload(UA_list_List* const list, UA_list_PayloadVisitor visitor);

UA_list_Element* UA_list_find(UA_list_List* const list, UA_list_PayloadMatcher matcher);

UA_list_Element* UA_list_search(UA_list_List* const list, UA_list_PayloadComparer compare, void* payload);

UA_list_Element* UA_list_getFirst(UA_list_List* const list);

UA_list_Element* UA_list_getLast(UA_list_List* const list);

#endif /* UA_LIST_H_ */
