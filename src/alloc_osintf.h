#pragma once

#include <stddef.h>
#include <string.h>

[[gnu::always_inline]]
inline void *alloc_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

[[gnu::always_inline]]
inline void *alloc_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}
