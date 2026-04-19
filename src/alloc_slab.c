#include <string.h>
#include <ytalloc/ytalloc.h>

#include "alloc_macros.h"

void alloc_slab_init(alloc_slab_t *heap, void *v_start, size_t size,
                     size_t alloc_size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(v_start != NULL);
    ASSERTF_ALWAYS((uintptr_t)v_start % alloc_size == 0,
                   "v_start must be aligned at alloc_size (%zu)", alloc_size);
    ASSERT_ALWAYS(size >= alloc_size);
    ASSERTF_ALWAYS(alloc_size >= sizeof(uintptr_t),
                   "alloc_size (%zu) must be greater than or equal to the size "
                   "of uintptr_t (%zu)",
                   alloc_size, sizeof(uintptr_t));

    memset(heap, 0, sizeof(alloc_slab_t));

    const size_t used_size =
        (size % alloc_size == 0) ? size : (size - size % alloc_size);

    heap->start = (uintptr_t)v_start;
    heap->end = heap->start + size;
    heap->used_size = used_size;
    heap->alloc_size = alloc_size;
    heap->num_items = used_size / alloc_size;

    for (size_t idx = 0; idx < size / alloc_size; idx++) {
        uintptr_t *const ptr_to_next = v_start + alloc_size * idx;
        if (idx + 1 == size / alloc_size) {
            *ptr_to_next = 0;
        } else {
            *ptr_to_next = (uintptr_t)ptr_to_next + alloc_size;
        }
    }
    heap->free_head = v_start;
}

void *alloc_slab(alloc_slab_t *heap) {
    ASSERT_DEBUG(heap != NULL);

    if (heap->free_head == NULL) {
        return NULL;
    } else {
        void *ptr = heap->free_head;
        const uintptr_t next = *heap->free_head;
        heap->free_head = (uintptr_t *)next;
        heap->num_used++;
        return ptr;
    }
}

void alloc_slab_free(alloc_slab_t *heap, void *ptr) {
    ASSERT_DEBUG(heap != NULL);
    if (!ptr) { return; }

    uintptr_t *const ptr_to_next = ptr;
    *ptr_to_next = (uintptr_t)heap->free_head;
    heap->free_head = ptr_to_next;

    ASSERT_ALWAYS(heap->num_used > 0);
    heap->num_used--;
}

size_t alloc_slab_num_free(const alloc_slab_t *heap) {
    ASSERT_DEBUG(heap != NULL);
    return heap->num_items - heap->num_used;
}

size_t alloc_slab_num_used(const alloc_slab_t *heap) {
    ASSERT_DEBUG(heap != NULL);
    return heap->num_used;
}

size_t alloc_slab_num_items(const alloc_slab_t *heap) {
    ASSERT_DEBUG(heap != NULL);
    return heap->num_items;
}
