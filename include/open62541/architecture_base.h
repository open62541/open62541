/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef ARCH_UA_ARCHITECTURE_BASE_H
#define ARCH_UA_ARCHITECTURE_BASE_H

/*
 * With the following list of defines, one can define its own UA_sleep_ms using a preprocessor define.
 * E.g. see unit tests.
 */

#ifdef UA_sleep_ms
void UA_sleep_ms(unsigned long ms);
#endif

#ifdef UA_malloc
void* UA_malloc(unsigned long size);
#endif

#ifdef UA_calloc
void* UA_calloc(unsigned long num, unsigned long size); //allocate memory in the heap with size*num bytes and set the memory to zero
#endif

#ifdef UA_realloc
void* UA_realloc(void *ptr, unsigned long new_size);//re-allocate memory in the heap with new_size bytes from previously allocated memory ptr
#endif

#ifdef UA_free
void UA_free(void* ptr); //de-allocate memory previously allocated with UA_malloc, UA_calloc or UA_realloc
#endif

#endif //ARCH_UA_ARCHITECTURE_BASE_H
