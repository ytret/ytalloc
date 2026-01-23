#pragma once

#include "alloc_osintf.h" // IWYU pragma: keep

#define PRINTS_ALWAYS(X)            PRINTF_IMPL("%s", X)
#define PRINTF_ALWAYS(FMT, ...)     PRINTF_IMPL(FMT, __VA_ARGS__)
#define ASSERT_ALWAYS(X)            ASSERT_IMPL(X, false, "%s", "")
#define ASSERTF_ALWAYS(X, FMT, ...) ASSERT_IMPL(X, true, FMT, __VA_ARGS__)

#ifdef YTALLOC_DEBUG
#define PRINTS_DEBUG(X)            PRINTF_IMPL("%s", X)
#define PRINTF_DEBUG(FMT, ...)     PRINTF_IMPL(FMT, __VA_ARGS__)
#define ASSERT_DEBUG(X)            ASSERT_IMPL(X, false, "%s", "")
#define ASSERTF_DEBUG(X, FMT, ...) ASSERT_IMPL(X, true, FMT, __VA_ARGS__)
#else
#define PRINTS_DEBUG(X)
#define PRINTF_DEBUG(FMT, ...)
#define ASSERT_DEBUG(X)
#define ASSERTF_DEBUG(x, FMT, ...)
#endif

#define PRINTF_IMPL(FMT, ...)                                                  \
    do {                                                                       \
        alloc_printf("" FMT "\n", __VA_ARGS__);                                \
    } while (0)

#define ASSERT_IMPL(X, HAS_MSG, FMT, ...)                                      \
    do {                                                                       \
        if (!(X)) {                                                            \
            PRINTS_ALWAYS("Assertion failed:");                                \
            if (HAS_MSG) { PRINTF_ALWAYS(FMT, __VA_ARGS__); }                  \
            PRINTF_ALWAYS("  Expression: %s", "" #X);                          \
            PRINTF_ALWAYS("        File: %s", __FILE__);                       \
            PRINTF_ALWAYS("    Function: %s", __FUNCTION__);                   \
            PRINTF_ALWAYS("        Line: %u", __LINE__);                       \
            YTALLOC_ABORT();                                                   \
        }                                                                      \
    } while (0)
