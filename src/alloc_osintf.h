#pragma once

#include <stddef.h>
#include <string.h>

#ifndef YTALLOC_MEMCPY
#define YTALLOC_MEMCPY memcpy
#endif

#ifndef YTALLOC_MEMSET
#define YTALLOC_MEMSET memset
#endif

[[gnu::always_inline]]
inline void *alloc_memcpy(void *dest, const void *src, size_t n) {
    return YTALLOC_MEMCPY(dest, src, n);
}

[[gnu::always_inline]]
inline void *alloc_memset(void *s, int c, size_t n) {
    return YTALLOC_MEMSET(s, c, n);
}
