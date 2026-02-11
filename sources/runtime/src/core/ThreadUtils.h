#pragma once

#include <string>

namespace Goonya {

enum class ThreadType {
    UNKNOWN = 0,
    LOGIC = 1,
    RENDER = 2,
    WORKER = 3,
};

inline thread_local ThreadType current_thread_type = ThreadType::UNKNOWN;

void set_current_thread_name(const std::string &name);

} // namespace Goonya
