
#include "UA_config.h"
#include "UA_list.h"

void UA_list_defaultFreer(void* payload){
	if(payload){
		opcua_free(payload);
	}
}
Int32 UA_list_initElement(UA_list_Element* const element){
	if(element==NULL)return UA_ERROR;
	element->next=NULL;
	element->prev=NULL;
	element->father=NULL;
	element->payload=NULL;
	return UA_NO_ERROR;
}

Int32 UA_list_init(UA_list_List* const list){
	if(list==NULL)return UA_ERROR;
	list->first = NULL;
	list->last = NULL;
	list->size = 0;
	return UA_NO_ERROR;
}

Int32 UA_list_addElementToFront(UA_list_List* const list, UA_list_Element* const element){
	if(list==NULL || element==NULL)return UA_ERROR;
	UA_list_Element* second = NULL;
	second = list->first;
	list->first = element;
	element->prev = NULL;
	element->next = second;
	element->father = list;
	if(second){
		second->prev=element;
	}
	list->size++;
	if(list->size==1){
		list->last=element;
	}
	return UA_NO_ERROR;
}

Int32 UA_list_addPayloadToFront(UA_list_List* const list, void* const payload){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* elem = (UA_list_Element*)opcua_malloc(sizeof(*elem));
	UA_list_initElement(elem);
	elem->payload = payload;
	UA_list_addElementToFront(list, elem);
	return UA_NO_ERROR;
}

Int32 UA_list_addElementToBack(UA_list_List* const list, UA_list_Element* const element){
	if(list==NULL || element == NULL)return UA_ERROR;
	UA_list_Element* secondLast = NULL;
	secondLast = list->last;
	list->last = element;
	element->prev = secondLast;
	element->next = NULL;
	element->father = list;
	if(secondLast){
		secondLast->next = element;
	}
	list->size++;
	if(list->size==1){
		list->first=element;
	}
	return UA_NO_ERROR;
}

Int32 UA_list_addPayloadToBack(UA_list_List* const list, void* const payload){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* elem = (UA_list_Element*)opcua_malloc(sizeof(*elem));
	UA_list_initElement(elem);
	elem->payload = payload;
	UA_list_addElementToBack(list, elem);
	return UA_NO_ERROR;
}

Int32 UA_list_removeFirst(UA_list_List* const list, UA_list_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* temp = NULL;
	if(list->first){
		temp = list->first->next;
		if(visitor){
			(*visitor)(list->first->payload);
		}
		opcua_free(list->first);
		list->first = temp;
		list->size--;
		if(list->size == 1){
			list->last = temp;
		}else if(list->size==0){
			list->last = NULL;
		}
	}
	return UA_NO_ERROR;
}

Int32 UA_list_removeLast(UA_list_List* const list, UA_list_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* temp = NULL;
	if(list->last){
		temp = list->last->prev;
		if(visitor){
			(*visitor)(list->last->payload);
		}
		opcua_free(list->last);
		list->last = temp;
		list->size--;
		if(list->size == 1){
			list->first = temp;
		}else if(list->size==0){
			list->first = NULL;
		}
	}
	return UA_NO_ERROR;
}


Int32 UA_list_removeElement(UA_list_Element* const elem, UA_list_PayloadVisitor visitor){
	if(elem==NULL)return UA_ERROR;
	if(elem==elem->father->first){
		return UA_list_removeFirst(elem->father, visitor);
	}else if(elem==elem->father->last){
		return UA_list_removeLast(elem->father, visitor);
	}else{
		UA_list_Element* prev = elem->prev;
		UA_list_Element* next = elem->next;
		prev->next = next;
		next->prev = prev;
		if(visitor){
			(*visitor)(elem->payload);
		}
		(elem->father)->size--;
		opcua_free(elem);
	}
	return UA_NO_ERROR;
}

Int32 UA_list_destroy(UA_list_List* const list, UA_list_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* current = NULL;
	current=list->first;
	while(current){
		UA_list_Element* next = current->next;
		if(visitor){
			(*visitor)(current->payload);
		}
		opcua_free(current);
		current = next;
	}
	UA_list_init(list);
	return UA_NO_ERROR;
}

Int32 UA_list_iterateElement(UA_list_List* const list, UA_list_ElementVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* current = list->first;
	while(current){
		if(visitor){
			(*visitor)(current);
		}
		current=current->next;
	}
	return UA_NO_ERROR;
}

/*Int32 UA_list_iteratePayload(UA_list_list* const list, UA_payloadVisitor visitor){
	void visitorTemp(UA_list_element* element){
		if(visitor){
			(*visitor)(element->payload);
		}
	}
	if(list==NULL)return UA_ERROR;
	UA_list_iterateElement(list, visitorTemp);
	return UA_NO_ERROR;
}*/
/** ANSI C forbids function nesting - reworked ugly version **/
Int32 UA_list_iteratePayload(UA_list_List* const list, UA_list_PayloadVisitor visitor){
	if(list==NULL)return UA_ERROR;
	UA_list_Element* current = list->first;
	while(current){
		if(visitor){
			(*visitor)(current->payload);
		}
		current=current->next;
	}
	return UA_NO_ERROR;
}

UA_list_Element* UA_list_find(UA_list_List* const list, UA_list_PayloadMatcher matcher){
	if(list==NULL)return NULL;
	if(matcher){
		UA_list_Element* current = list->first;
		while(current){
			if(matcher && (*matcher)(current->payload)==TRUE){
				return current;
			}
			current=current->next;
		}
	}
	/* nothing found */
	return NULL;
}

UA_list_Element* UA_list_getFirst(UA_list_List* const list){
	if(list==NULL)return NULL;
	return list->first;
}

UA_list_Element* UA_list_getLast(UA_list_List* const list){
	if(list==NULL)return NULL;
	return list->last;
}
