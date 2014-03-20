#include "UA_config.h"
#include "UA_indexedList.h"

void UA_indexedList_defaultFreer(void* payload){
	UA_list_defaultFreer(payload);
}

Int32 UA_indexedList_init(UA_indexedList_List* const list){
	if(list==NULL)return UA_ERROR;
	return UA_list_init((UA_list_List*)list);
}

Int32 UA_indexedList_destroy(UA_indexedList_List* const list, UA_indexedList_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* current = NULL;
	current=list->first;
	while(current){
		UA_list_Element* next = current->next;
		UA_indexedList_Element* elem = (UA_indexedList_Element*)current->payload;
		if(visitor){
			(*visitor)(elem->payload);
		}
		if(elem){
			free(elem);
		}
		free(current);
		current = next;
	}
	UA_list_init(list);
	return UA_NO_ERROR;
}

Int32 UA_indexedList_initElement(UA_indexedList_Element* const elem){
	if(elem==NULL)return UA_ERROR;
	elem->father = NULL;
	elem->index = -1;
	elem->payload = NULL;
	return UA_NO_ERROR;
}

Int32 UA_indexedList_addValue(UA_indexedList_List* const list, Int32 index, void* payload){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* dllElem = (UA_list_Element*)malloc(sizeof(*dllElem));
	UA_list_initElement(dllElem);
	UA_indexedList_Element* iilElem = (UA_indexedList_Element*)malloc(sizeof(UA_indexedList_Element));
	UA_indexedList_initElement(iilElem);
	iilElem->index = index;
	iilElem->father = dllElem;
	iilElem->payload = payload;
	dllElem->payload = iilElem;
	return UA_list_addElementToBack((UA_list_List*)list, dllElem);
}

Int32 UA_indexedList_addValueToFront(UA_indexedList_List* const list, Int32 index, void* payload){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* dllElem = (UA_list_Element*)malloc(sizeof(*dllElem));
	UA_list_initElement(dllElem);
	UA_indexedList_Element* iilElem = (UA_indexedList_Element*)malloc(sizeof(*iilElem));
	UA_indexedList_initElement(iilElem);
	iilElem->index = index;
	iilElem->father = dllElem;
	iilElem->payload = payload;
	dllElem->payload = iilElem;
	return UA_list_addElementToFront((UA_list_List*)list, dllElem);
}

UA_indexedList_Element* UA_indexedList_find(UA_indexedList_List* const list, Int32 index){
	if(list==NULL)return NULL;
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
	return NULL;
}

void* UA_indexedList_findValue(UA_indexedList_List* const list, Int32 index){
	if(list==NULL)return NULL;
	UA_indexedList_Element* iilElem = UA_indexedList_find(list, index);
	if(iilElem){
		return iilElem->payload;
	}
	return NULL;
}

Int32 UA_indexedList_iterateValues(UA_indexedList_List* const list, UA_indexedList_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
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
	return UA_NO_ERROR;
}

Int32 UA_indexedList_removeElement(UA_indexedList_List* const list, UA_indexedList_Element* elem, UA_indexedList_PayloadVisitor visitor){
	if(list==NULL || elem==NULL)return UA_ERROR;
	if(visitor){
		(*visitor)(elem->payload);
	}
	UA_list_Element* father = elem->father;
	free(elem);
	return UA_list_removeElement(father, NULL);
}
