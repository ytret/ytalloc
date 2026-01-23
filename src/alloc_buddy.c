#include <limits.h>
#include <ytalloc/ytalloc.h>

#include "alloc_macros.h"
#include "alloc_osintf.h"

typedef struct alloc_buddy_tag {
    bool used;

    union {
        struct {
            // used = true
            uint8_t order;
        };
        struct {
            // used = false
            struct alloc_buddy_tag *prev;
            struct alloc_buddy_tag *next;
        };
    };

#if SIZE_MAX == UINT32_MAX
    uint8_t reserved[18];
#else
    uint8_t reserved[8];
#endif
} alloc_buddy_tag_t;

static_assert(sizeof(alloc_buddy_tag_t) == YTALLOC_BUDDY_TAG_SIZE);

static uint64_t prv_alloc_calc_log2(uint64_t num);
static size_t prv_alloc_calc_num_orders(size_t heap_size,
                                        size_t *out_min_block_size);
static size_t prv_alloc_calc_block_order(const alloc_buddy_t *heap,
                                         size_t alloc_size);
static size_t prv_alloc_calc_pow2_ge(size_t num);

static void *prv_alloc_get_free_block(alloc_buddy_t *heap, size_t order);
static void prv_alloc_add_free_block(alloc_buddy_t *heap, uintptr_t block,
                                     uint8_t order);
static uintptr_t prv_alloc_get_buddy(const alloc_buddy_t *heap, uintptr_t block,
                                     size_t order);

void alloc_buddy_init(alloc_buddy_t *heap, void *v_start, size_t size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(v_start != NULL);

    ASSERTF_ALWAYS(size >= YTALLOC_BUDDY_MIN_BLOCK_SIZE, "size must be >= %d",
                   YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    const uintptr_t start = (uintptr_t)v_start;

    const size_t size_log2 = prv_alloc_calc_log2(size);
    const size_t rounded_size = (size_t)1 << size_log2;
    ASSERTF_ALWAYS(size_log2 > 0, "size %zu is too small", size);
    ASSERTF_ALWAYS(
        (start & (rounded_size - 1)) == 0,
        "v_start is misaligned for a heap of size %zu (%zu after rounding)",
        size, rounded_size);

    size_t min_block_size;
    const size_t num_orders =
        prv_alloc_calc_num_orders(rounded_size, &min_block_size);

    alloc_memset(heap, 0, sizeof(*heap));
    heap->start = start;
    heap->end = start + size;
    heap->used_size = rounded_size;
    heap->min_block_size = min_block_size;
    heap->num_orders = num_orders;

    alloc_buddy_tag_t *const biggest_block = v_start;
    biggest_block->prev = NULL;
    biggest_block->next = NULL;
    ASSERT_DEBUG(num_orders > 0);
    heap->free_heads[num_orders - 1] = (uintptr_t)biggest_block;
}

void *alloc_buddy(alloc_buddy_t *heap, size_t size) {
    ASSERT_DEBUG(heap != NULL);

    if (size == 0) { return NULL; }
    if (size > heap->used_size) { return NULL; }

    const size_t order = prv_alloc_calc_block_order(heap, size);
    void *const ptr = prv_alloc_get_free_block(heap, order);
    if (ptr) {
        return ptr + YTALLOC_BUDDY_TAG_SIZE;
    } else {
        return NULL;
    }
}

void alloc_buddy_free(alloc_buddy_t *heap, void *ptr) {
    ASSERT_DEBUG(heap != NULL);
    if (!ptr) { return; }

    const uintptr_t block = (uintptr_t)ptr - YTALLOC_BUDDY_TAG_SIZE;
    ASSERTF_DEBUG(heap->start <= (uintptr_t)block && (uintptr_t)ptr < heap->end,
                  "%s", "ptr is outside the heap");

    alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)block;

    ASSERT_ALWAYS(tag->used);
    const uint8_t order = tag->order;
    tag->used = false;
    tag->prev = NULL;
    tag->next = NULL;

    prv_alloc_add_free_block(heap, block, order);
}

static size_t prv_alloc_calc_log2(size_t num) {
    size_t log2 = 0;
    while (num >>= 1) {
        log2++;
    }
    return log2;
}

static size_t prv_alloc_calc_num_orders(size_t heap_size,
                                        size_t *out_min_block_size) {
    ASSERT_DEBUG((heap_size & (heap_size - 1)) == 0);

    size_t num_orders = 0;
    size_t block_size = heap_size;
    while (block_size >= YTALLOC_BUDDY_MIN_BLOCK_SIZE) {
        num_orders++;
        block_size /= 2;
    }

    *out_min_block_size = block_size * 2;

    if (num_orders >= YTALLOC_BUDDY_MAX_ORDERS) {
        num_orders = YTALLOC_BUDDY_MAX_ORDERS;
    }

    return num_orders;
}

/**
 * Calculates the minimum block order that suits the specified allocation size.
 */
static size_t prv_alloc_calc_block_order(const alloc_buddy_t *heap,
                                         size_t alloc_size) {
    const size_t need_size = YTALLOC_BUDDY_TAG_SIZE + alloc_size;
    const size_t size_pow2 = prv_alloc_calc_pow2_ge(need_size);
    if (size_pow2 <= heap->min_block_size) { return 0; }
    const size_t order = prv_alloc_calc_log2(size_pow2 / heap->min_block_size);
    return order;
}

/**
 * Rounds @a num up to the next highest power of two.
 * @note
 * If the next highest power of two for @a num is bigger than `SIZE_MAX`, then
 * this function returns `0`.
 */
static size_t prv_alloc_calc_pow2_ge(size_t num) {
    if (num == 0) { return 0; }
    if ((num & (num - 1)) == 0) { return num; }

#if SIZE_MAX == UINT32_MAX
    if ((num & (1U << 31U)) != 0) { return 0; }
#else
    if ((num & (1ULL << 63ULL)) != 0) { return 0; }
#endif

    num--;
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;
#if SIZE_MAX == UINT64_MAX
    num |= num >> 32;
#endif
    ASSERT_DEBUG(num < SIZE_MAX);
    num++;

    return num;
}

/**
 * Returns the first free block of the specified order.
 *
 * - Splits the higher order block if needed.
 * - Marks the block as used and removes it from the free list.
 *
 * @returns Pointer to the allocated block or `NULL` if no block could be found.
 */
static void *prv_alloc_get_free_block(alloc_buddy_t *heap, size_t order) {
    if (order >= heap->num_orders) { return NULL; }

    uintptr_t free_block = heap->free_heads[order];
    if (free_block == 0) {
        void *const higher_block = prv_alloc_get_free_block(heap, order + 1);
        if (!higher_block) { return NULL; }

        alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)higher_block;
        tag->used = true;
        tag->order = order;

        alloc_buddy_tag_t *const buddy_tag =
            (alloc_buddy_tag_t *)prv_alloc_get_buddy(
                heap, (uintptr_t)higher_block, order);
        buddy_tag->used = false;
        buddy_tag->prev = NULL;
        buddy_tag->next = NULL;
        heap->free_heads[order] = (uintptr_t)buddy_tag;

        return higher_block;
    } else {
        alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)free_block;

        ASSERTF_DEBUG(
            tag->prev == NULL,
            "heap->free_heads[%zu] is supposed to point at the first tag",
            order);
        heap->free_heads[order] = (uintptr_t)tag->next;
        if (tag->next) { tag->next->prev = tag->prev; }

        tag->used = true;
        tag->order = order;

        return (void *)free_block;
    }
}

/**
 * Adds the specified block to the free block list.
 *
 * - Marks the block as free.
 * - Adds it to the free list.
 * - Merges the block with its buddy if the buddy is free too.
 *
 * @param heap  Buddy heap struct pointer.
 * @param block Address of the block.
 * @param order Order of the block.
 */
static void prv_alloc_add_free_block(alloc_buddy_t *heap, uintptr_t block,
                                     uint8_t order) {
    alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)block;

    ASSERT_DEBUG(heap->num_orders > 0);
    const bool has_buddy = order < (heap->num_orders - 1);

    if (has_buddy) {
        const uintptr_t buddy = prv_alloc_get_buddy(heap, block, order);
        const alloc_buddy_tag_t *const buddy_tag =
            (const alloc_buddy_tag_t *)buddy;

        if (buddy_tag->used) {
            alloc_buddy_tag_t *const head =
                (alloc_buddy_tag_t *)heap->free_heads[order];
            if (head) {
                ASSERT_DEBUG(!head->used);
                head->prev = tag;
            }
            tag->next = head;
            heap->free_heads[order] = block;
        } else {
            if (buddy_tag->prev) {
                buddy_tag->prev->next = buddy_tag->next;
            } else {
                heap->free_heads[order] = (uintptr_t)buddy_tag->next;
            }
            if (buddy_tag->next) { buddy_tag->next->prev = buddy_tag->prev; }

            const uintptr_t higher_block = block < buddy ? block : buddy;
            prv_alloc_add_free_block(heap, higher_block, order + 1);
        }
    } else {
        heap->free_heads[order] = block;
    }
}

static uintptr_t prv_alloc_get_buddy(const alloc_buddy_t *heap, uintptr_t block,
                                     size_t order) {
    const size_t block_size = heap->min_block_size * ((size_t)1U << order);
    ASSERT_DEBUG((block & (block_size - 1)) == 0);
    return block ^ block_size;
}
