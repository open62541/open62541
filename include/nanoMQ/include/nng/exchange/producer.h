#ifndef PRODUCER_H
#define PRODUCER_H

#include<stddef.h>

typedef struct producer_s producer_t;
struct producer_s {
	/*
	 * return: 0: success, -1: failed
	 */
	int (*match)(void *data);
	/*
	 * return: 0: continue, -1: stop and return
	 */
	int (*target)(void *data);
};

int producer_init(producer_t **p,
				  int (*match)(void *data),
				  int (*target)(void *data));
int producer_release(producer_t *p);

#endif
