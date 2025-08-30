#pragma once

#include <stdio.h>  // IWYU pragma: keep
#include <stdlib.h> // IWYU pragma: keep

#define D_PRINTS(X)            D_PRINTF_IMPL("%s", X)
#define D_PRINTF(FMT, ...)     D_PRINTF_IMPL(FMT, __VA_ARGS__)
#define D_ASSERT(X)            D_ASSERT_IMPL(X, false, "%s", "")
#define D_ASSERTF(X, FMT, ...) D_ASSERT_IMPL(X, true, FMT, __VA_ARGS__)

#ifdef YTALLOC_DEBUG

#define D_PRINTF_IMPL(FMT, ...)                                                \
    do {                                                                       \
        fprintf(stderr, "" FMT "\n", __VA_ARGS__);                             \
    } while (0)

#define D_ASSERT_IMPL(X, HAS_MSG, FMT, ...)                                    \
    do {                                                                       \
        if (!(X)) {                                                            \
            D_PRINTS("Assertion failed:");                                     \
            if (HAS_MSG) { D_PRINTF(FMT, __VA_ARGS__); }                       \
            D_PRINTF("  Expression: %s", "" #X);                               \
            D_PRINTF("        File: %s", __FILE__);                            \
            D_PRINTF("    Function: %s", __FUNCTION__);                        \
            D_PRINTF("        Line: %u", __LINE__);                            \
            abort();                                                           \
        }                                                                      \
    } while (0)

#else

#define D_PRINTF_IMPL(FMT, ...)                                                \
    do {                                                                       \
    } while (0);

#define D_ASSERT_IMPL(X, HAS_MSG, FMT, ...)                                    \
    do {                                                                       \
    } while (0);

#endif
