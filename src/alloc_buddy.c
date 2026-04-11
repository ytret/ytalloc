#include <limits.h>
#include <ytalloc/ytalloc.h>

#include "alloc_macros.h"
#include "alloc_osintf.h"
#include "aux/auxmath.h"

typedef struct alloc_buddy_tag {
    struct alloc_buddy_tag *prev;
    struct alloc_buddy_tag *next;
} alloc_buddy_tag_t;

static size_t prv_alloc_calc_num_orders(size_t heap_size,
                                        size_t *out_min_block_size);
static size_t prv_alloc_calc_block_order(const alloc_buddy_t *heap,
                                         size_t alloc_size);

static void *prv_alloc_get_free_block(alloc_buddy_t *heap, size_t order);
static void *prv_alloc_get_free_aligned_block(alloc_buddy_t *heap,
                                              size_t size_order,
                                              size_t alignment);
static void prv_alloc_add_free_block(alloc_buddy_t *heap, uintptr_t block,
                                     uint8_t order);
static uintptr_t prv_alloc_get_buddy(const alloc_buddy_t *heap, uintptr_t block,
                                     size_t order);

static bool prv_alloc_is_block_used(const alloc_buddy_t *heap, uintptr_t block);
static void prv_alloc_set_block_used(alloc_buddy_t *heap, uintptr_t block,
                                     bool used);

void alloc_buddy_init(alloc_buddy_t *heap, void *v_start, size_t size,
                      void *free_heads, size_t free_heads_size, void *bitmap,
                      size_t bitmap_size) {
    ASSERT_ALWAYS(heap != NULL);
    ASSERT_ALWAYS(v_start != NULL);
    ASSERT_ALWAYS(free_heads != NULL);

    ASSERTF_ALWAYS(size >= YTALLOC_BUDDY_MIN_BLOCK_SIZE, "size must be >= %d",
                   YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    const uintptr_t start = (uintptr_t)v_start;

    const size_t size_log2 = alloc_calc_log2(size);
    const size_t rounded_size = (size_t)1 << size_log2;
    ASSERTF_ALWAYS(size_log2 > 0, "size %zu is too small", size);
    ASSERTF_ALWAYS(
        (start & (rounded_size - 1)) == 0,
        "v_start is misaligned for a heap of size %zu (%zu after rounding)",
        size, rounded_size);

    size_t min_block_size;
    const size_t num_orders =
        prv_alloc_calc_num_orders(rounded_size, &min_block_size);

    ASSERTF_ALWAYS(free_heads_size >= sizeof(uintptr_t) * num_orders,
                   "free_heads_size must be >= %zu",
                   sizeof(uintptr_t) * num_orders);
    alloc_memset(free_heads, 0, free_heads_size);

    const size_t num_order0_blocks = rounded_size / min_block_size;
    const size_t need_bitmap_size = ((num_order0_blocks + 7) & ~7) / 8;
    ASSERTF_ALWAYS(bitmap_size >= need_bitmap_size,
                   "bitmap_size must be >= %zu", need_bitmap_size);
    alloc_memset(bitmap, 0, bitmap_size);

    alloc_memset(heap, 0, sizeof(*heap));
    heap->start = start;
    heap->end = start + size;
    heap->used_size = rounded_size;
    heap->min_block_size = min_block_size;
    heap->num_orders = num_orders;
    heap->free_heads = free_heads;
    heap->usage_bitmap = bitmap;
    heap->bitmap_size = bitmap_size;

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
    return prv_alloc_get_free_block(heap, order);
}

void *alloc_buddy_aligned(alloc_buddy_t *heap, size_t size, size_t align) {
    ASSERT_DEBUG(heap != NULL);

    if (size == 0) { return NULL; }
    if (size > heap->used_size) { return NULL; }

    const size_t size_order = prv_alloc_calc_block_order(heap, size);
    const size_t align_order = prv_alloc_calc_block_order(heap, align);
    return prv_alloc_get_free_aligned_block(heap, size_order, align_order);
}

void alloc_buddy_free(alloc_buddy_t *heap, void *ptr, size_t size) {
    ASSERT_DEBUG(heap != NULL);
    if (!ptr) { return; }

    const uintptr_t block = (uintptr_t)ptr;
    ASSERTF_DEBUG(heap->start <= (uintptr_t)block && (uintptr_t)ptr < heap->end,
                  "%s", "ptr is outside the heap");

    const size_t order = prv_alloc_calc_block_order(heap, size);
    ASSERT_DEBUG(order < heap->num_orders);

    const bool block_is_used = prv_alloc_is_block_used(heap, block);
    ASSERT_ALWAYS(block_is_used);
    prv_alloc_set_block_used(heap, block, false);

    alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)block;
    tag->prev = NULL;
    tag->next = NULL;

    prv_alloc_add_free_block(heap, block, order);
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
    const size_t size_pow2 = alloc_calc_pow2_ge(alloc_size);
    if (size_pow2 <= heap->min_block_size) { return 0; }
    const size_t order = alloc_calc_log2(size_pow2 / heap->min_block_size);
    return order;
}

/**
 * Allocates a block of the specified order.
 *
 * - Splits a higher order block if needed.
 * - Marks the allocated block as used and removes it from the free list.
 *
 * @param heap  Heap structure pointer.
 * @param order Order of the block to allocate.
 *
 * @returns Pointer to the allocated block or `NULL` if no block could be found.
 */
static void *prv_alloc_get_free_block(alloc_buddy_t *heap, size_t order) {
    if (order >= heap->num_orders) { return NULL; }

    const uintptr_t free_block = heap->free_heads[order];
    if (free_block == 0) {
        // No block that satisfies the size and alignment requirements. Split a
        // higher order block.

        void *const higher_block =
            prv_alloc_get_free_aligned_block(heap, order + 1, 1);
        if (!higher_block) { return NULL; }

        // We will use its left child and make the other one (`buddy`) free.
        const uintptr_t buddy =
            prv_alloc_get_buddy(heap, (uintptr_t)higher_block, order);
        alloc_buddy_tag_t *const buddy_tag = (alloc_buddy_tag_t *)buddy;
        buddy_tag->prev = NULL;
        buddy_tag->next = NULL;
        prv_alloc_set_block_used(heap, buddy, false);
        heap->free_heads[order] = (uintptr_t)buddy_tag;

        return higher_block;
    } else {
        // We found a block of the required order. Remove it from the free list.

        alloc_buddy_tag_t *const tag = (alloc_buddy_tag_t *)free_block;

        ASSERTF_DEBUG(
            tag->prev == NULL,
            "heap->free_heads[%zu] is supposed to point at the first tag",
            order);
        heap->free_heads[order] = (uintptr_t)tag->next;
        if (tag->next) { tag->next->prev = tag->prev; }

        prv_alloc_set_block_used(heap, free_block, true);

        return (void *)free_block;
    }
}

/**
 * Allocates a suitable block of the specified order and alignment.
 *
 * Same as #prv_alloc_get_free_block(), except satisfies the alignment
 * requirement.
 *
 * @param heap          Heap structure pointer.
 * @param size_order    Order of the block to allocate.
 * @param align_order   Alignment order. The allocated block will be aligned at
 *                      the size of a block of this order.
 *
 * @returns Pointer to the allocated block or `NULL` if no block could be found.
 */
static void *prv_alloc_get_free_aligned_block(alloc_buddy_t *heap,
                                              size_t size_order,
                                              size_t align_order) {
    const size_t find_order =
        size_order >= align_order ? size_order : align_order;
    if (find_order >= heap->num_orders) { return NULL; }

    void *const block = prv_alloc_get_free_block(heap, find_order);
    if (!block) { return NULL; }

    if (size_order >= align_order) {
        // Any block of order `size_order` already satisfies the alignment
        // requirement.
        return block;
    }

    // Otherwise, we found a block that is bigger than required, but has the
    // proper alignment. Split it until we get the smallest suitable size.
    size_t order = align_order - 1;
    while (true) {
        // We always choose the left child, because it has the same alignment
        // as its parents. Here `buddy` is the right child.
        const uintptr_t buddy =
            prv_alloc_get_buddy(heap, (uintptr_t)block, order);

        prv_alloc_set_block_used(heap, (uintptr_t)block, true);

        // TODO: prv_alloc_add_free_block() checks if the buddy (of `buddy`,
        // here it is `block`) is used. In this case it is obviously not, so
        // maybe optimize?
        prv_alloc_add_free_block(heap, buddy, order);

        if (order == 0) { break; }
        order--;
    }

    return block;
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

    // Zero initialize the pointers. The actual values are set below.
    tag->prev = NULL;
    tag->next = NULL;

    if (has_buddy) {
        const uintptr_t buddy = prv_alloc_get_buddy(heap, block, order);
        const alloc_buddy_tag_t *const buddy_tag =
            (const alloc_buddy_tag_t *)buddy;

        if (prv_alloc_is_block_used(heap, buddy)) {
            alloc_buddy_tag_t *const head =
                (alloc_buddy_tag_t *)heap->free_heads[order];
            if (head) { head->prev = tag; }
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

static bool prv_alloc_is_block_used(const alloc_buddy_t *heap,
                                    uintptr_t block) {
    ASSERT_DEBUG(block >= heap->start);
    ASSERT_DEBUG(block < heap->end);

    const size_t abs_bit_pos = (block - heap->start) / heap->min_block_size;
    const size_t byte_pos = abs_bit_pos / 8;
    const size_t bit_pos = abs_bit_pos % 8;

    ASSERTF_ALWAYS(byte_pos < heap->bitmap_size, "%s",
                   "byte_pos is beyond bitmap size");

    const uint8_t usage_byte = heap->usage_bitmap[byte_pos];
    const bool usage_bit = (usage_byte & (1 << bit_pos)) != 0;

    return usage_bit;
}

static void prv_alloc_set_block_used(alloc_buddy_t *heap, uintptr_t block,
                                     bool used) {
    ASSERT_DEBUG(block >= heap->start);
    ASSERT_DEBUG(block < heap->end);

    const size_t abs_bit_pos = (block - heap->start) / heap->min_block_size;
    const size_t byte_pos = abs_bit_pos / 8;
    const size_t bit_pos = abs_bit_pos % 8;

    ASSERTF_ALWAYS(byte_pos < heap->bitmap_size, "%s",
                   "byte_pos is beyond bitmap size");

    if (used) {
        heap->usage_bitmap[byte_pos] |= 1 << bit_pos;
    } else {
        heap->usage_bitmap[byte_pos] &= ~(1 << bit_pos);
    }
}
