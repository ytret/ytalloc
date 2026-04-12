#pragma once

#include "alloc_osintf.h" // IWYU pragma: keep

#define LOG_ALWAYS(X)               LOGF_IMPL("%s", X)
#define LOGF_ALWAYS(FMT, ...)       LOGF_IMPL(FMT, __VA_ARGS__)
#define ASSERT_ALWAYS(X)            ASSERT_IMPL(X, false, "%s", "")
#define ASSERTF_ALWAYS(X, FMT, ...) ASSERT_IMPL(X, true, FMT, __VA_ARGS__)

#ifdef YTALLOC_DEBUG
#define LOG_DEBUG(X)               LOGF_IMPL("%s", X)
#define LOGF_DEBUG(FMT, ...)       LOGF_IMPL(FMT, __VA_ARGS__)
#define ASSERT_DEBUG(X)            ASSERT_IMPL(X, false, "%s", "")
#define ASSERTF_DEBUG(X, FMT, ...) ASSERT_IMPL(X, true, FMT, __VA_ARGS__)
#else
#define LOG_DEBUG(X)
#define LOGF_DEBUG(FMT, ...)
#define ASSERT_DEBUG(X)
#define ASSERTF_DEBUG(x, FMT, ...)
#endif

#define LOGF_IMPL(FMT, ...)                                                    \
    do {                                                                       \
        alloc_log("" FMT "\n", __VA_ARGS__);                                   \
    } while (0)

#define ASSERT_IMPL(X, HAS_MSG, FMT, ...)                                      \
    do {                                                                       \
        if (!(X)) {                                                            \
            LOG_ALWAYS("Assertion failed:");                                   \
            if (HAS_MSG) { LOGF_ALWAYS(FMT, __VA_ARGS__); }                    \
            LOGF_ALWAYS("  Expression: %s", "" #X);                            \
            LOGF_ALWAYS("        File: %s", __FILE__);                         \
            LOGF_ALWAYS("    Function: %s", __FUNCTION__);                     \
            LOGF_ALWAYS("        Line: %u", __LINE__);                         \
            alloc_abort();                                                     \
        }                                                                      \
    } while (0)
