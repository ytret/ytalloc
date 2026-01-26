#pragma once

#include <stddef.h>

size_t alloc_calc_log2(size_t num);

/**
 * Rounds @a num up to the next highest power of two.
 * @note
 * If the next highest power of two for @a num is bigger than `SIZE_MAX`, then
 * this function returns `0`.
 */
size_t alloc_calc_pow2_ge(size_t num);
