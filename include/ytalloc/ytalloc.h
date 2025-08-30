#pragma once

#include <stddef.h>
#include <stdint.h>

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

void alloc_list_init(alloc_list_t *heap, void *start, size_t size);
void *alloc_list(alloc_list_t *heap, size_t size);
void alloc_list_free(alloc_list_t *heap, void *ptr);

void alloc_static_init(alloc_static_t *heap, void *start, size_t size);
void *alloc_static(alloc_static_t *heap, size_t size);

#if __cplusplus
}
#endif
