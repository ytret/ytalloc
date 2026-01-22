#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef YTALLOC_BUDDY_MAX_ORDERS
#define YTALLOC_BUDDY_MAX_ORDERS 32
#endif
#ifndef YTALLOC_BUDDY_MIN_BLOCK_SIZE
#define YTALLOC_BUDDY_MIN_BLOCK_SIZE 32
#endif

#define YTALLOC_BUDDY_TAG_SIZE 32

static_assert(YTALLOC_BUDDY_MAX_ORDERS > 0);
static_assert(YTALLOC_BUDDY_MIN_BLOCK_SIZE > 0);

#if __cplusplus
extern "C" {
#endif

typedef struct list list_t;

typedef struct {
    uintptr_t start;
    uintptr_t end;

    list_t *tag_list;
} alloc_list_t;

typedef struct {
    uintptr_t start;
    uintptr_t end;

    uintptr_t next;
} alloc_static_t;

typedef struct {
    uintptr_t start;
    uintptr_t end;
    size_t used_size;

    size_t min_block_size;
    size_t num_orders;
    uintptr_t free_heads[YTALLOC_BUDDY_MAX_ORDERS];
} alloc_buddy_t;

void alloc_list_init(alloc_list_t *heap, void *start, size_t size);
void *alloc_list(alloc_list_t *heap, size_t size);
void alloc_list_free(alloc_list_t *heap, void *ptr);

void alloc_static_init(alloc_static_t *heap, void *start, size_t size);
void *alloc_static(alloc_static_t *heap, size_t size);

void alloc_buddy_init(alloc_buddy_t *heap, void *start, size_t size);
void *alloc_buddy(alloc_buddy_t *heap, size_t size);
void alloc_buddy_free(alloc_buddy_t *heap, void *ptr);

#if __cplusplus
}
#endif
