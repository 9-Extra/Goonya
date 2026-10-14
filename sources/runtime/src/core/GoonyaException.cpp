#include "runtime/GoonyaException.h"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>
#include <stacktrace>
#include <string>
#include <string_view>

#include "core/format_exception.h"
#include "core/log/Log.h"

namespace Goonya {

namespace {

void report(std::string_view text) noexcept {
    if (core_logger) {
        try {
            LOG_ERROR("{}", text);
            // 日志走的是异步队列，这里紧接着就要abort，必须等队列排空消息才发得出去
            Logger::shutdown();
            return;
        } catch (...) {
        }
    } else {
        std::fwrite(text.data(), 1, text.size(), stderr);
        std::fputc('\n', stderr);
        std::fflush(stderr);
    }
}

std::string describe_pending_exception() {
    std::string information;
    std::exception_ptr exception = std::current_exception();
    if (!exception) {
        information += "std::terminate被调用，但没有异常（可能是代码直接调用了std::terminate）\n";
        information += std::format("Stacktrace:\n{}\n", std::stacktrace::current(1));
        return information;
    }

    information += "std::terminate被调用，异常无法传播到最外层：\n";
    try {
        std::rethrow_exception(exception);
    } catch (const std::exception &e) {
        information += format_exception(e);
    } catch (...) {
        information += "未知类型的异常（非std::exception）\n";
    }
    return information;
}

void terminate_handler() {
    std::string information;
    try {
        information = describe_pending_exception();
    } catch (...) {
        information = "std::terminate被调用，且整理异常信息时又出现了错误\n";
    }
    report(information);

#ifdef _MSC_VER
    // 静止微软的CRT在abort时会弹一个纯属碍事对话框
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    std::abort();
}

} // namespace

void install_terminate_handler() { std::set_terminate(&terminate_handler); }

} // namespace Goonya
