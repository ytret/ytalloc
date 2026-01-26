#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef YTALLOC_BUDDY_MAX_ORDERS
#define YTALLOC_BUDDY_MAX_ORDERS 12
#endif
#ifndef YTALLOC_BUDDY_MIN_BLOCK_SIZE
#define YTALLOC_BUDDY_MIN_BLOCK_SIZE 4096
#endif

#define YTALLOC_BUDDY_MIN_ALLOC_SIZE YTALLOC_BUDDY_MIN_BLOCK_SIZE

static_assert(YTALLOC_BUDDY_MAX_ORDERS > 0);
static_assert(YTALLOC_BUDDY_MIN_BLOCK_SIZE > 0);
static_assert(YTALLOC_BUDDY_MIN_ALLOC_SIZE > 0);

#if __cplusplus
extern "C" {
#endif

typedef struct list list_t;

typedef struct {
    uintptr_t start;
    uintptr_t end;

    list_t *tag_list;

#if SIZE_MAX == UINT32_MAX
    [[gnu::aligned(4)]] uint8_t prv[8];
#else
    [[gnu::aligned(8)]] uint8_t prv[16];
#endif
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
    uintptr_t *free_heads;
    uint8_t *usage_bitmap;
    size_t bitmap_size;
} alloc_buddy_t;

typedef struct {
    uintptr_t start;
    uintptr_t end;
    size_t used_size;

    size_t alloc_size;
    uintptr_t *free_head;
} alloc_slab_t;

void alloc_list_init(alloc_list_t *heap, void *start, size_t size);
void *alloc_list(alloc_list_t *heap, size_t size);
void alloc_list_free(alloc_list_t *heap, void *ptr);

void alloc_static_init(alloc_static_t *heap, void *start, size_t size);
void *alloc_static(alloc_static_t *heap, size_t size);

void alloc_buddy_init(alloc_buddy_t *heap, void *start, size_t size,
                      void *free_heads, size_t free_heads_size, void *bitmap,
                      size_t bitmap_size);
void *alloc_buddy(alloc_buddy_t *heap, size_t size);
void alloc_buddy_free(alloc_buddy_t *heap, void *ptr, size_t size);

void alloc_slab_init(alloc_slab_t *heap, void *start, size_t size,
                     size_t alloc_size);
void *alloc_slab(alloc_slab_t *heap);
void alloc_slab_free(alloc_slab_t *heap, void *ptr);

#if __cplusplus
}
#endif
