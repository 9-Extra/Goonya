#pragma once

#include <spdlog/logger.h>
#include <thread>

#include "core/ThreadPool.h"
#include "opengl/OpenGLAPI.h" // IWYU: pragma export

namespace Goonya {

class OpenGLGraphicsAPI;

extern OpenGLGraphicsAPI GL;
extern std::thread render_thread;

template <typename T, bool IS_RHI_THREAD = false>
void enqueue_render_task(T &&task) {
    if constexpr (IS_RHI_THREAD){
        task();
    } else {
        THREAD_POOL.enqueue_renderer_thread(std::forward<T>(task));
    }
}

} // namespace Goonya
