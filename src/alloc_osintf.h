#pragma once

#include <stdarg.h>
#include <stddef.h>

void *YTALLOC_MEMCPY(void *dest, const void *src, size_t n);
void *YTALLOC_MEMSET(void *s, int c, size_t n);
void YTALLOC_ABORT(void);
int YTALLOC_VPRINTF(const char *restrict fmt, va_list ap);

[[gnu::always_inline]]
inline void *alloc_memcpy(void *dest, const void *src, size_t n) {
    return YTALLOC_MEMCPY(dest, src, n);
}

[[gnu::always_inline]]
inline void *alloc_memset(void *s, int c, size_t n) {
    return YTALLOC_MEMSET(s, c, n);
}

int alloc_printf(const char *fmt, ...);
