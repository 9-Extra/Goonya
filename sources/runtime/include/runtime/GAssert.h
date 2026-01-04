#pragma once

#include <exception>       // IWYU pragma: keep
#include <format>          // IWYU pragma: keep
#include <iostream>        // IWYU pragma: keep
#include <source_location> // IWYU pragma: keep
#include <stacktrace>      // IWYU pragma: keep

#include "core/log/Log.h" // IWYU pragma: keep

#ifdef DEBUG
#define GN_ENABLE_ASSERT
#endif

#ifdef GN_ENABLE_ASSERT

#define GN_ASSERT_MSG(cond, ...)                                                                                       \
    if (!(cond)) [[unlikely]] {                                                                                        \
        ::std::source_location location = ::std::source_location::current();                                           \
        ::std::stacktrace trace = std::stacktrace::current(0);                                                         \
        LOG_ERROR("{} at {}:{} ({})\nStacktrace:\n{}", ::std::format(__VA_ARGS__), location.file_name(),               \
                  location.line(), location.function_name(), trace);                                                   \
        spdlog::drop_all();                                                                                            \
        std::cout.flush();                                                                                             \
        std::cerr.flush();                                                                                             \
        std::terminate();                                                                                              \
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