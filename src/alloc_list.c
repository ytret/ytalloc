#include <ytalloc/ytalloc.h>

#include "alloc_macros.h"
#include "alloc_osintf.h"
#include "aux/list.h"

#define ALLOC_LIST_MIN_SIZE 64

typedef struct {
    list_node_t node;
    bool used;
    uintptr_t start;
    size_t size;
} alloc_tag_t;

static alloc_tag_t *prv_alloc_list_find(alloc_list_t *heap, void *chunk_start);

[[maybe_unused]] static void prv_alloc_list_check(alloc_list_t *heap);
static bool prv_alloc_list_check_node(alloc_list_t *heap, list_node_t *node);
static bool prv_alloc_list_check_tag(alloc_list_t *heap, alloc_tag_t *tag);
static bool prv_alloc_list_check_addr(alloc_list_t *heap, uintptr_t addr);

void alloc_list_init(alloc_list_t *heap, void *start, size_t size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(start != NULL);

    constexpr size_t min_size = sizeof(alloc_tag_t);

    ASSERTF_ALWAYS(size >= min_size, "size %zu is too small, need at least %zu",
                   size, min_size);
    ASSERTF_ALWAYS((uintptr_t)start % alignof(alloc_tag_t) == 0,
                   "start %p has bad alignment for type alloc_tag_t", start);

    alloc_memset(heap, 0, sizeof(*heap));

    static_assert(sizeof(heap->prv) == sizeof(ytaux_list_t));
    static_assert(offsetof(alloc_list_t, prv) % _Alignof(ytaux_list_t) == 0);

    heap->start = (uintptr_t)start;
    heap->end = heap->start + size;

    heap->tag_list = (ytaux_list_t *)&heap->prv[0];
    list_init(heap->tag_list, NULL);

    // Create a tag for the free chunk that is the most part of the heap.
    alloc_tag_t *const tag = (alloc_tag_t *)heap->start;
    alloc_memset(tag, 0, sizeof(*tag));
    tag->used = false;
    tag->start = heap->start + sizeof(alloc_tag_t);
    tag->size = heap->end - tag->start;
    list_append(heap->tag_list, &tag->node);

    if (tag->size < ALLOC_LIST_MIN_SIZE) {
        PRINTF_DEBUG("alloc_list_init: free chunk size (%zu) is less than the "
                     "minimum allocation size (%u)",
                     tag->size, ALLOC_LIST_MIN_SIZE);
    }
}

void *alloc_list(alloc_list_t *heap, size_t size) {
    ASSERT_DEBUG(heap != NULL);
    ASSERT_DEBUG(heap->tag_list != NULL);

    if (size < ALLOC_LIST_MIN_SIZE) { size = ALLOC_LIST_MIN_SIZE; }

#if ALLOC_LIST_DO_CHECKS
    prv_alloc_list_check(heap);
#endif

    alloc_tag_t *found_tag = NULL;
    for (list_node_t *node = heap->tag_list->p_first_node; node != NULL;
         node = node->p_next) {
        alloc_tag_t *const tag = LIST_NODE_TO_STRUCT(node, alloc_tag_t, node);
        if (tag->used) { continue; }
        if (tag->size < size) { continue; }
        found_tag = tag;
        break;
    }
    if (!found_tag) {
        PRINTF_DEBUG("alloc_list: could not find a free tag for an allocation "
                     "of size %zu",
                     size);
        return NULL;
    }

    const size_t extra_size = found_tag->size - size;
    if (extra_size > sizeof(alloc_tag_t) + ALLOC_LIST_MIN_SIZE) {
        alloc_tag_t *const new_tag = (alloc_tag_t *)(found_tag->start + size);
        alloc_memset(new_tag, 0, sizeof(*new_tag));
        new_tag->used = false;
        new_tag->start = (uintptr_t)new_tag + sizeof(alloc_tag_t);
        new_tag->size = found_tag->size - size - sizeof(alloc_tag_t);

        found_tag->size = size;

        list_insert(heap->tag_list, &found_tag->node, &new_tag->node);
    }

#if ALLOC_LIST_DO_CHECKS
    prv_alloc_list_check(heap);
#endif

    found_tag->used = true;
    return (void *)found_tag->start;
}

void alloc_list_free(alloc_list_t *heap, void *ptr) {
    ASSERT_DEBUG(heap != NULL);

    if (!ptr) { return; }

#if ALLOC_LIST_DO_CHECKS
    prv_alloc_list_check(heap);
#endif

    alloc_tag_t *const tag = prv_alloc_list_find(heap, ptr);
    ASSERTF_ALWAYS(tag != NULL,
                   "alloc_free: could not find a chunk that starts at %p", ptr);

    tag->used = false;

#if ALLOC_LIST_DO_CHECKS
    prv_alloc_list_check(heap);
#endif
}

static alloc_tag_t *prv_alloc_list_find(alloc_list_t *heap, void *chunk_start) {
    ASSERT_DEBUG(heap != NULL);
    ASSERT_DEBUG(heap->tag_list != NULL);

    for (list_node_t *node = heap->tag_list->p_first_node; node != NULL;
         node = node->p_next) {
        alloc_tag_t *const tag = LIST_NODE_TO_STRUCT(node, alloc_tag_t, node);
        if ((uintptr_t)chunk_start == tag->start) { return tag; }
    }

    return NULL;
}

static void prv_alloc_list_check(alloc_list_t *heap) {
    ASSERT_DEBUG(heap != NULL);

    list_node_t *const first_node = heap->tag_list->p_first_node;
    ASSERTF_ALWAYS(prv_alloc_list_check_node(heap, first_node),
                   "bad first node of the heap at %p", heap);

    size_t idx = 0;
    for (list_node_t *node = first_node; node != NULL; node = node->p_next) {
        alloc_tag_t *const tag = LIST_NODE_TO_STRUCT(node, alloc_tag_t, node);
        ASSERTF_ALWAYS(prv_alloc_list_check_tag(heap, tag),
                       "bad tag #%zu at %p", idx, tag);
        idx++;
    }
}

static bool prv_alloc_list_check_node(alloc_list_t *heap, list_node_t *node) {
    ASSERT_DEBUG(heap != NULL);
    ASSERT_DEBUG(node != NULL);

    if (prv_alloc_list_check_addr(heap, (uintptr_t)node)) {
        return true;
    } else {
        PRINTF_ALWAYS("alloc_list: bad node pointer %p", node);
        return false;
    }
}

static bool prv_alloc_list_check_tag(alloc_list_t *heap, alloc_tag_t *tag) {
    ASSERT_DEBUG(heap != NULL);
    ASSERT_DEBUG(tag != NULL);

    if (!prv_alloc_list_check_addr(heap, tag->start)) {
        PRINTF_ALWAYS(
            "alloc_list: bad tag at %p: start address %p is out of bounds", tag,
            (void *)tag->start);
        return false;
    }

    if (!prv_alloc_list_check_addr(heap, tag->start + tag->size - 1)) {
        PRINTF_ALWAYS(
            "alloc_list: bad tag at %p: end address %p is out of bounds", tag,
            (void *)(tag->start + tag->size));
        return false;
    }

    return true;
}

static bool prv_alloc_list_check_addr(alloc_list_t *heap, uintptr_t addr) {
    ASSERT_DEBUG(heap != NULL);
    return heap->start <= addr && addr < heap->end;
}
