#include "Graphics.h"

#include <memory>

#include "function/renderer/RendererBasic.h"
#include "opengl/OpenGLAPI.h"
#include "runtime/GoonyaException.h"

namespace Goonya::Graphics {

std::unique_ptr<GraphicsAPI> graphics_api;
std::queue<std::function<void()>> render_tasks;

void initialize(GraphicsAPIType api_type) {
    ASSERT_RENDER_THREAD();
    switch (api_type) {
    case GraphicsAPIType::NONE:
        throw RuntimeError("不行");
    case GraphicsAPIType::OPENGL: {
        graphics_api = std::make_unique<OpenGLGraphicsAPI>();
        break;
    };
    default:
        throw RuntimeError("不支持的API类型");
    }
}

void run_all_tasks() {
    ASSERT_RENDER_THREAD();
    // 运行所有推入的Task
    while (!render_tasks.empty()) {
        std::invoke(std::move(render_tasks.front()));
        render_tasks.pop();
    }
}

void drop() { graphics_api.reset(); }

} // namespace Goonya::Graphics
