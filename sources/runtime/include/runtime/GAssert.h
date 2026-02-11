#pragma once

#include <exception>       // IWYU pragma: keep
#include <format>          // IWYU pragma: keep
#include <iostream>        // IWYU pragma: keep
#include <source_location> // IWYU pragma: keep
#include <stacktrace>      // IWYU pragma: keep

namespace Goonya::Details {
// at sources\runtime\src\core\GAssert.cpp
[[noreturn]] void _log_and_exit(const std::string &msg, const std::source_location &location);
}

#ifdef DEBUG
#define GN_ENABLE_ASSERT
#endif

#ifdef GN_ENABLE_ASSERT

#define GN_ASSERT_MSG(cond, ...)                                                                                       \
    if (!(cond)) [[unlikely]] {                                                                                        \
        ::Goonya::Details::_log_and_exit(::std::format(__VA_ARGS__), ::std::source_location::current());                                                 \
    } else                                                                                                             \
        (void(0))

#define GN_ASSERT(cond) GN_ASSERT_MSG(cond, "Assertion failed: {}", #cond)

#else
#define GN_ASSERT_MSG(cond, ...)                                                                                       \
    do {                                                                                                               \
        bool x = bool(cond);                                                                                           \
        [[assume(x)]];                                                                                                 \
    } while (false)
#define GN_ASSERT(cond)                                                                                                \
    do {                                                                                                               \
        bool x = bool(cond);                                                                                           \
        [[assume(x)]];                                                                                                 \
    } while (false)
#endif