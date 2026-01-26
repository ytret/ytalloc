#include "alloc_macros.h"
#include "alloc_osintf.h"
#include "ytalloc/ytalloc.h"

static_assert((YTALLOC_STATIC_ALIGN == 1) || (YTALLOC_STATIC_ALIGN == 2) ||
              (YTALLOC_STATIC_ALIGN == 4) || (YTALLOC_STATIC_ALIGN == 8) ||
              (YTALLOC_STATIC_ALIGN == 16) || (YTALLOC_STATIC_ALIGN == 32));

void alloc_static_init(alloc_static_t *heap, void *start, size_t size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(start != NULL);
    ASSERTF_ALWAYS((uintptr_t)start % YTALLOC_STATIC_ALIGN == 0,
                   "start must be %d-byte aligned", YTALLOC_STATIC_ALIGN);

    alloc_memset(heap, 0, sizeof(*heap));

    heap->start = (uintptr_t)start;
    heap->end = heap->start + size;

    heap->next = heap->start;
}

void *alloc_static(alloc_static_t *heap, size_t size) {
    ASSERT_DEBUG(heap != NULL);

    if (size == 0) { return NULL; }

    uintptr_t new_next = heap->next + size;
    new_next =
        (new_next + (YTALLOC_STATIC_ALIGN - 1)) & ~(YTALLOC_STATIC_ALIGN - 1);
    if (new_next > heap->end) { return NULL; }

    void *const ptr = (void *)heap->next;
    heap->next = new_next;

    return ptr;
}
