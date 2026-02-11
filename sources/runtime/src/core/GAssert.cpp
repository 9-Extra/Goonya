#include "runtime/GAssert.h"

#include "core/log/Log.h"

namespace Goonya::Details {

[[noreturn]] void _log_and_exit(const std::string &msg, const std::source_location &location) {
    LOG_ERROR("{} at {}:{} ({})\nStacktrace:\n{}", msg, location.file_name(), location.line(), location.function_name(),
              std::stacktrace::current(1));
    spdlog::drop_all();
    std::cout.flush();
    std::cerr.flush();
    std::terminate();
}

} // namespace Goonya
