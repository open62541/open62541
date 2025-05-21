#ifndef QUEUE_H
#define QUEUE_H
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_SIZE 5

typedef struct {
    void** array;
    int front;
    int rear;
    int size;
    int capacity;
} CircularQueue;


#define INIT_QUEUE(queue) \
    do { \
        (queue).array = (void**)malloc(INITIAL_SIZE * sizeof(void*)); \
        (queue).front = -1; \
        (queue).rear = -1; \
        (queue).size = 0; \
        (queue).capacity = INITIAL_SIZE; \
    } while(0)

#define NEXT_INDEX(index, size) (((index) + 1) % (size))

#define IS_EMPTY(queue) ((queue).size == 0)

#define QUEUE_SIZE(queue) ((queue).size)

#define ENQUEUE(queue, element) \
    do { \
        if ((queue).size == (queue).capacity) { \
            int new_capacity = (queue).capacity * 2; \
            void** new_array = (void**)malloc(new_capacity * sizeof(void*)); \
            int i = 0; \
            int j = (queue).front; \
            while (i < (queue).size) { \
                new_array[i] = (queue).array[j]; \
                i++; \
                j = NEXT_INDEX(j, (queue).capacity); \
            } \
            free((queue).array); \
            (queue).array = new_array; \
            (queue).front = 0; \
            (queue).rear = (queue).size - 1; \
            (queue).capacity = new_capacity; \
        } \
        if (IS_EMPTY(queue)) { \
            (queue).front = 0; \
        } \
        (queue).rear = NEXT_INDEX((queue).rear, (queue).capacity); \
        (queue).array[(queue).rear] = (element); \
        (queue).size++; \
    } while(0)

#define DEQUEUE(queue) \
    ({ \
        void* dequeued_element = NULL; \
        if (IS_EMPTY(queue)) { \
            printf("Queue is empty. Cannot dequeue.\n"); \
        } else { \
            dequeued_element = (queue).array[(queue).front]; \
            (queue).front = NEXT_INDEX((queue).front, (queue).capacity); \
            (queue).size--; \
            if (IS_EMPTY(queue)) { \
                (queue).front = -1; \
                (queue).rear = -1; \
            } \
        } \
        dequeued_element; \
    })


#define FOREACH_QUEUE(queue, elem) \
    for (int i = (queue).front, _count = 0; \
         (queue).size != 0 && _count < (queue).size; \
         i = NEXT_INDEX(i, (queue).capacity), _count++) \
    \
        for (elem = (queue).array[i]; \
             elem != NULL; \
             elem = NULL)

#define DESTROY_QUEUE(queue) free((queue).array)

#endif