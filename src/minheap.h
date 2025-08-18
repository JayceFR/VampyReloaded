#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdbool.h>

typedef struct pathNode* pathNode; // forward declaration

typedef struct {
    pathNode *data;
    int size;
    int capacity;
} MinHeap;

// Create and destroy
MinHeap* minheap_create(int capacity);
void minheap_free(MinHeap *heap);

// Core functionality
void minheap_push(MinHeap *heap, pathNode node);
pathNode minheap_pop(MinHeap *heap);
pathNode minheap_peek(MinHeap *heap);

bool minheap_is_empty(MinHeap *heap);
void minheap_clear(MinHeap *heap);

#endif
