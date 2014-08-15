#include "ua_list.h"
#include "ua_util.h"

void UA_list_defaultFreer(void* payload){
	if(payload){
		UA_free(payload);
	}
}

UA_Int32 UA_list_initElement(UA_list_Element* element){
	if(element==UA_NULL) return UA_ERROR;
	element->next=UA_NULL;
	element->prev=UA_NULL;
	element->father=UA_NULL;
	element->payload=UA_NULL;
	return UA_NO_ERROR;
}

UA_Int32 UA_list_init(UA_list_List* list){
	if(list==UA_NULL) return UA_ERROR;
	list->first = UA_NULL;
	list->last = UA_NULL;
	list->size = 0;
	return UA_NO_ERROR;
}

UA_Int32 UA_list_addElementToFront(UA_list_List* list, UA_list_Element* element){
	UA_list_Element* second = UA_NULL;
	if(list==UA_NULL || element==UA_NULL) return UA_ERROR;
	second = list->first;
	list->first = element;
	element->prev = UA_NULL;
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

UA_Int32 UA_list_addPayloadToFront(UA_list_List* list, void* const payload) {
	UA_list_Element* elem;
	if(list==UA_NULL)
		return UA_ERROR;
	UA_alloc((void**)&elem, sizeof(*elem));
	UA_list_initElement(elem);
	elem->payload = payload;
	UA_list_addElementToFront(list, elem);
	return UA_NO_ERROR;
}

UA_Int32 UA_list_addElementToBack(UA_list_List* list, UA_list_Element* element) {
	UA_list_Element* secondLast = UA_NULL;
	if(list==UA_NULL || element == UA_NULL)
		return UA_ERROR;
	secondLast = list->last;
	list->last = element;
	element->prev = secondLast;
	element->next = UA_NULL;
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

UA_Int32 UA_list_addPayloadToBack(UA_list_List* list, void* const payload) {
	UA_list_Element* elem;
	if(list==UA_NULL)
		return UA_ERROR;
	UA_alloc((void**)&elem, sizeof(*elem));
	UA_list_initElement(elem);
	elem->payload = payload;
	UA_list_addElementToBack(list, elem);
	return UA_NO_ERROR;
}

UA_Int32 UA_list_removeFirst(UA_list_List* list, UA_list_PayloadVisitor visitor) {
	UA_list_Element* temp = UA_NULL;
	if(list==UA_NULL)
		return UA_ERROR;
	if(list->first){
		temp = list->first->next;
		if(visitor){
			(*visitor)(list->first->payload);
		}
		UA_free(list->first);
		list->first = temp;
		if(temp){
			temp->prev = UA_NULL;
		}
		list->size--;
		if(list->size == 1){
			list->last = temp;
		}else if(list->size==0){
			list->last = UA_NULL;
		}
	}
	return UA_NO_ERROR;
}

UA_Int32 UA_list_removeLast(UA_list_List* list, UA_list_PayloadVisitor visitor) {
	UA_list_Element* temp = UA_NULL;
	if(list==UA_NULL)
		return UA_ERROR;
	if(list->last){
		temp = list->last->prev;
		if(visitor){
			(*visitor)(list->last->payload);
		}
		UA_free(list->last);
		list->last = temp;
		if(temp){
			temp->next = UA_NULL;
		}
		list->size--;
		if(list->size == 1){
			list->first = temp;
		}else if(list->size==0){
			list->first = UA_NULL;
		}
	}
	return UA_NO_ERROR;
}

UA_Int32 UA_list_removeElement(UA_list_Element* const elem, UA_list_PayloadVisitor visitor) {
	if(elem==UA_NULL)
		return UA_ERROR;
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
		UA_free(elem);
	}
	return UA_NO_ERROR;
}

UA_Int32 UA_list_destroy(UA_list_List* list, UA_list_PayloadVisitor visitor) {
	UA_list_Element* current = UA_NULL;
	if(list==UA_NULL)
		return UA_ERROR;
	current=list->first;
	while(current){
		UA_list_Element* next = current->next;
		if(visitor){
			(*visitor)(current->payload);
		}
		UA_free(current);
		current = next;
	}
	UA_list_init(list);
	return UA_NO_ERROR;
}

UA_Int32 UA_list_iterateElement(UA_list_List* const list, UA_list_ElementVisitor visitor) {
	UA_list_Element* current;
	UA_list_Element* next = UA_NULL;
	if(list==UA_NULL)
		return UA_ERROR;
	current = list->first;
	while(current){
		next=current->next;
		if(visitor){
			(*visitor)(current);
		}
		current = next;
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
UA_Int32 UA_list_iteratePayload(UA_list_List* const list, UA_list_PayloadVisitor visitor){
	UA_list_Element* current;
	UA_list_Element* next = UA_NULL;
	if(list==UA_NULL)
		return UA_ERROR;
	current = list->first;
	while(current){
		next = current->next;
		if(visitor){
			(*visitor)(current->payload);
		}
		current = next;
	}
	return UA_NO_ERROR;
}

UA_list_Element* UA_list_find(UA_list_List* const list, UA_list_PayloadMatcher matcher){
	if(list==UA_NULL)return UA_NULL;
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
	return UA_NULL;
}

UA_list_Element* UA_list_search(UA_list_List* const list, UA_list_PayloadComparer compare, void* payload){
	if(list==UA_NULL)return UA_NULL;
	if(compare){
		UA_list_Element* current = list->first;
		while(current){
			if(compare && (*compare)(current->payload, payload)==TRUE){
				return current;
			}
			current=current->next;
		}
	}
	/* nothing found */
	return UA_NULL;
}

UA_list_Element* UA_list_getFirst(UA_list_List* const list){
	if(list==UA_NULL)return UA_NULL;
	return list->first;
}

UA_list_Element* UA_list_getLast(UA_list_List* const list){
	if(list==UA_NULL)return UA_NULL;
	return list->last;
}
