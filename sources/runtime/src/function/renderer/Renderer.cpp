#include "Renderer.h"

#include "core/ThreadUtils.h"
#include "function/renderer/Pipeline.h"

namespace Goonya {
Renderer renderer; // global renderer

void Renderer::init() { render_pipeline = std::make_unique<Pipeline>(); }

void Renderer::render() {

    renderer_thread_process();
    render_pipeline->render();
}

void Renderer::clear() {
    if (current_thread_type == ThreadType::RENDER) {
        renderer_thread_process();
    }
    // todo: 我们无法确认在清空这些资源时，是否会有其他的对象还持有引用
    camera_set.clear();
}

} // namespace Goonya
