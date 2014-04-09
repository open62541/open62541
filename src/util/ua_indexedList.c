#include "ua_indexedList.h"

void UA_indexedList_defaultFreer(void* payload){
	UA_list_defaultFreer(payload);
}

UA_Int32 UA_indexedList_init(UA_indexedList_List* list){
	if(list==UA_NULL)return UA_ERROR;
	return UA_list_init((UA_list_List*)list);
}

UA_Int32 UA_indexedList_destroy(UA_indexedList_List* list, UA_indexedList_PayloadVisitor visitor){
	if(list==UA_NULL)return UA_ERROR;
	UA_list_Element* current = UA_NULL;
	current=list->first;
	while(current){
		UA_list_Element* next = current->next;
		UA_indexedList_Element* elem = (UA_indexedList_Element*)current->payload;
		if(visitor){
			(*visitor)(elem->payload);
		}
		if(elem){
			UA_free(elem);
		}
		UA_free(current);
		current = next;
	}
	UA_list_init(list);
	return UA_NO_ERROR;
}

UA_Int32 UA_indexedList_initElement(UA_indexedList_Element* elem){
	if(elem==UA_NULL)return UA_ERROR;
	elem->father = UA_NULL;
	elem->index = -1;
	elem->payload = UA_NULL;
	return UA_NO_ERROR;
}

UA_Int32 UA_indexedList_addValue(UA_indexedList_List* list, UA_Int32 index, void* payload){
	if(list==UA_NULL)return UA_ERROR;
	UA_list_Element* dllElem;
	UA_alloc((void**)&dllElem, sizeof(UA_list_Element));
	UA_list_initElement(dllElem);
	UA_indexedList_Element* iilElem;
	UA_alloc((void**)&iilElem, sizeof(UA_indexedList_Element));
	UA_indexedList_initElement(iilElem);
	iilElem->index = index;
	iilElem->father = dllElem;
	iilElem->payload = payload;
	dllElem->payload = iilElem;
	return UA_list_addElementToBack((UA_list_List*)list, dllElem);
}

UA_Int32 UA_indexedList_addValueToFront(UA_indexedList_List* list, UA_Int32 index, void* payload) {
	if(list==UA_NULL)return UA_ERROR;
	UA_list_Element* dllElem;
	UA_alloc((void**)&dllElem, sizeof(UA_list_Element));
	UA_list_initElement(dllElem);
	UA_indexedList_Element* iilElem;
	UA_alloc((void**)&iilElem, sizeof(UA_indexedList_Element));
	UA_indexedList_initElement(iilElem);
	iilElem->index = index;
	iilElem->father = dllElem;
	iilElem->payload = payload;
	dllElem->payload = iilElem;
	return UA_list_addElementToFront((UA_list_List*)list, dllElem);
}

UA_indexedList_Element* UA_indexedList_find(UA_indexedList_List* const list, UA_Int32 index){
	if(list==UA_NULL)return UA_NULL;
	UA_list_Element* current = list->first;
	while(current){
		if(current->payload){
			UA_indexedList_Element* elem = (UA_indexedList_Element*)current->payload;
			if(elem->index == index){
				return elem;
			}
		}
		current=current->next;
	}
	return UA_NULL;
}

void* UA_indexedList_findValue(UA_indexedList_List* const list, UA_Int32 index){
	if(list==UA_NULL)return UA_NULL;
	UA_indexedList_Element* iilElem = UA_indexedList_find(list, index);
	if(iilElem){
		return iilElem->payload;
	}
	return UA_NULL;
}

UA_Int32 UA_indexedList_iterateValues(UA_indexedList_List* const list, UA_indexedList_PayloadVisitor visitor){
	if(list==UA_NULL)return UA_ERROR;
	UA_list_Element* current = list->first;
	while(current){
		if(current->payload){
			UA_indexedList_Element* elem = (UA_indexedList_Element*)current->payload;
			if(visitor){
				(*visitor)(elem->payload);
			}
		}
		current=current->next;
	}
	return UA_NO_ERROR;
}

UA_Int32 UA_indexedList_removeElement(UA_indexedList_List* list, UA_indexedList_Element* elem, UA_indexedList_PayloadVisitor visitor){
	if(list==UA_NULL || elem==UA_NULL)return UA_ERROR;
	if(visitor){
		(*visitor)(elem->payload);
	}
	UA_list_Element* father = elem->father;
	UA_free(elem);
	return UA_list_removeElement(father, UA_NULL);
}
