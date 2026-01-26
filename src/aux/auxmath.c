#include <stdint.h>

#include "aux/auxmath.h"

size_t alloc_calc_log2(size_t num) {
    size_t log2 = 0;
    while (num >>= 1) {
        log2++;
    }
    return log2;
}

size_t alloc_calc_pow2_ge(size_t num) {
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
    num++;

    return num;
}
