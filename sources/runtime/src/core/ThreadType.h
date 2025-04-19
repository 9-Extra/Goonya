#pragma once

namespace Goonya {

enum class ThreadType{
    UNKNOWN = 0,
    LOGIC = 1,
    RENDER = 2,
};

inline thread_local ThreadType current_thread_type = ThreadType::UNKNOWN;

} // namespace Goonya