#include "alloc_macros.h"
#include "alloc_osintf.h"
#include "ytalloc/ytalloc.h"

void alloc_static_init(alloc_static_t *heap, void *start, size_t size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(start != NULL);

    alloc_memset(heap, 0, sizeof(*heap));

    heap->start = (uintptr_t)start;
    heap->end = heap->start + size;

    heap->next = heap->start;
}

void *alloc_static(alloc_static_t *heap, size_t size) {
    ASSERT_DEBUG(heap != NULL);

    if (size == 0) { return NULL; }

    const uintptr_t new_next = heap->next + size;
    if (new_next > heap->end) { return NULL; }

    void *const ptr = (void *)heap->next;
    heap->next = new_next;

    return ptr;
}
