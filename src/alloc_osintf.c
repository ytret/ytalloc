#include <ytalloc/ytalloc.h>

#include "alloc_osintf.h"

static alloc_log_fn g_alloc_log_fn;
static alloc_abort_fn g_alloc_abort_fn;

void alloc_set_log_fn(alloc_log_fn fn) {
    g_alloc_log_fn = fn;
}

void alloc_set_abort_fn(alloc_abort_fn fn) {
    g_alloc_abort_fn = fn;
}

int alloc_log(const char *fmt, ...) {
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    if (g_alloc_log_fn) { ret = g_alloc_log_fn(fmt, ap); }
    va_end(ap);

    return ret;
}

void alloc_abort(void) {
    if (g_alloc_abort_fn) { g_alloc_abort_fn(); }
    __builtin_trap();
}
