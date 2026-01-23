#pragma once

#include <stddef.h>

#ifdef YTALLOC_HAS_STDLIB
#include <stdlib.h>
#include <string.h>
#endif

#ifndef YTALLOC_MEMCPY
#define YTALLOC_MEMCPY memcpy
#endif

#ifndef YTALLOC_MEMSET
#define YTALLOC_MEMSET memset
#endif

#ifndef YTALLOC_ABORT
#define YTALLOC_ABORT abort
#endif

[[gnu::always_inline]]
inline void *alloc_memcpy(void *dest, const void *src, size_t n) {
    return YTALLOC_MEMCPY(dest, src, n);
}

[[gnu::always_inline]]
inline void *alloc_memset(void *s, int c, size_t n) {
    return YTALLOC_MEMSET(s, c, n);
}
