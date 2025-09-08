#include "format_exception.h"

#include "runtime/GoonyaException.h"

#include <format>
#include <iterator>

namespace Goonya {

static void format_exception(const std::exception &e, int level, std::string &information) noexcept {
    std::format_to(std::back_inserter(information), "{:{}}Exception: {}\n", "", level, e.what());
    try {
        std::rethrow_if_nested(e);
        // not nested exception, the last one
// 仅在调试模式下启动堆栈回溯
#ifdef DEBUG
        const RuntimeError *error = dynamic_cast<const RuntimeError *>(&e);
        if (error) {
            std::format_to(std::back_inserter(information), "Stacktrace:\n{}", error->get_trace());

        } else {
            std::format_to(std::back_inserter(information), "Get exception type \"{}\". No stacktrace available",
                           typeid(e).name());
        }
#endif
    } catch (const std::exception &nestedException) {
        format_exception(nestedException, level + 1, information);
    } catch (...) {
        std::format_to(std::back_inserter(information), "{:{}} Unknown exception type: \"{}\"", "", level,
                       typeid(std::current_exception()).name());
    }
}

std::string format_exception(const std::exception &e) noexcept {
    std::string information;
    format_exception(e, 0, information);
    return information;
}

} // namespace Goonya