#pragma once

#include <cstddef>
#include <stacktrace>
#include <stdexcept>

namespace Goonya {

class RuntimeError : public std::runtime_error {
    std::stacktrace trace;

public:
// 仅在调试模式下启动堆栈回溯
#ifdef DEBUG
#define CURRENT_BACKTRACE(skip) std::stacktrace::current(skip)
#else
#define CURRENT_BACKTRACE(skip)                                                                                        \
    std::stacktrace {}
#endif

    explicit RuntimeError(const std::string &msg, size_t skip = 1) noexcept
        : std::runtime_error{msg}, trace{CURRENT_BACKTRACE(skip)} {};
    explicit RuntimeError(const char *msg, size_t skip = 1) noexcept
        : std::runtime_error{msg}, trace{CURRENT_BACKTRACE(skip)} {};

    const std::stacktrace &get_trace() const noexcept { return trace; }

#undef CURRENT_BACKTRACE
};

// 与std::throw_with_nested一同使用，不需要带有std::stacktrace
class RuntimeErrorNest : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

} // namespace Goonya