#include "format_exception.h"

#include "runtime/GoonyaException.h"

#include <format>
#include <iterator>

namespace Goonya {

static void format_exception(const std::exception &e, int level, std::string &information) noexcept {
    try {
        std::format_to(std::back_inserter(information), "{:{}}Exception: {}\n", "", level, e.what());
    } catch (...) {
        information += "Exception: [format failed]";
    }

    try {
        std::rethrow_if_nested(e);
        // not nested exception, the last one
// 仅在调试模式下启动堆栈回溯
#ifdef DEBUG
        const RuntimeError *error = dynamic_cast<const RuntimeError *>(&e);
        try {
            if (error) {
                std::format_to(std::back_inserter(information), "Stacktrace:\n{}", error->get_trace());

            } else {
                std::format_to(std::back_inserter(information), "Get exception type \"{}\". No stacktrace available",
                               typeid(e).name());
            }
        } catch (...) {
            information += "Stacktrace: [format failed]";
        }
#endif
    } catch (const std::exception &nestedException) {
        format_exception(nestedException, level + 1, information);
    } catch (...) {
        try {
            std::format_to(std::back_inserter(information), "{:{}} Unknown exception type: \"{}\"", "", level,
                           typeid(std::current_exception()).name());
        } catch (...) {
            information += "Unknown exception: [format failed]";
        }
    }
}

std::string format_exception(const std::exception &e) noexcept {
    std::string information;
    information.reserve(512);
    format_exception(e, 0, information);
    return information;
}

} // namespace Goonya