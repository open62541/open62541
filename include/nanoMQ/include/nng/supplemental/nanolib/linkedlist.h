#include <stdio.h>
#include <stdlib.h>

struct linkedListNode {
	void *data;
	unsigned long long expiredAt;
	struct linkedListNode *next;
	struct linkedListNode *prev;
};

struct linkedList {
	unsigned int cap;
	unsigned int size;
	unsigned int overWrite;
	unsigned long long expiredAt;

	struct linkedListNode *head;
	struct linkedListNode *tail;
};

int linkedList_replace_head(struct linkedList *list,
							void *data,
							unsigned long long expiredAt);
int linkedList_init(struct linkedList **list,
					unsigned int cap,
					unsigned int overWrite,
					unsigned long long expiredAt);
int linkedList_enqueue(struct linkedList *list,
					   void *data,
					   unsigned long long expiredAt);
int linkedList_dequeue(struct linkedList *list,
					   void **data);
int linkedList_release(struct linkedList *list);
