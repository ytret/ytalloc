#pragma once

int alloc_log(const char *fmt, ...);
[[gnu::noreturn]] void alloc_abort(void);
