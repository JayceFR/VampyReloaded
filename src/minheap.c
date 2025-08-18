#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "map.h"
#include "minheap.h"

static void swap(pathNode *a, pathNode *b) {
    pathNode temp = *a;
    *a = *b;
    *b = temp;
}

static void heapify_up(MinHeap *heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (heap->data[parent]->fCost <= heap->data[index]->fCost)
            break;
        swap(&heap->data[parent], &heap->data[index]);
        index = parent;
    }
}

static void heapify_down(MinHeap *heap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < heap->size && heap->data[left]->fCost < heap->data[smallest]->fCost)
        smallest = left;
    if (right < heap->size && heap->data[right]->fCost < heap->data[smallest]->fCost)
        smallest = right;

    if (smallest != index) {
        swap(&heap->data[index], &heap->data[smallest]);
        heapify_down(heap, smallest);
    }
}

MinHeap* minheap_create(int capacity) {
    MinHeap *heap = malloc(sizeof(MinHeap));
    assert(heap != NULL);
    heap->data = malloc(sizeof(pathNode) * capacity);
    assert(heap->data != NULL);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void minheap_free(MinHeap *heap) {
    if (heap) {
        free(heap->data);
        free(heap);
    }
}

void minheap_push(MinHeap *heap, pathNode node) {
    assert(heap->size < heap->capacity);
    heap->data[heap->size] = node;
    heapify_up(heap, heap->size);
    heap->size++;
}

pathNode minheap_pop(MinHeap *heap) {
    if (heap->size == 0) return NULL;
    pathNode top = heap->data[0];
    heap->size--;
    heap->data[0] = heap->data[heap->size];
    heapify_down(heap, 0);
    return top;
}

pathNode minheap_peek(MinHeap *heap) {
    return heap->size > 0 ? heap->data[0] : NULL;
}

bool minheap_is_empty(MinHeap *heap) {
    return heap->size == 0;
}

void minheap_clear(MinHeap *heap) {
    heap->size = 0;
}
