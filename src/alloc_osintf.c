#include "alloc_osintf.h"

int alloc_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int ret = YTALLOC_VPRINTF(fmt, args);
    va_end(args);
    return ret;
}
