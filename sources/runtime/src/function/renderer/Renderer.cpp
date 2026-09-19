#include "Renderer.h"

#include "core/ThreadPool.h"
#include "function/renderer/Pipeline.h"
#include "imgui.h"

namespace Goonya {
Renderer renderer; // global renderer

void Renderer::init() { render_pipeline = std::make_unique<Pipeline>(); }

void Renderer::render() {
    renderer_thread_process();
    render_pipeline->render();

    if (ImGui::Begin("Rendering")) {
        ImGui::SeparatorText("Config");
        // 切换必须发生在帧边界上
        ImGui::Checkbox("计算着色器蒙皮", &skinning_on_gpu);
        ImGui::Checkbox("辉光后处理", &draw_bloom);
    }
    ImGui::End();
}

RScene *Renderer::create_scene() noexcept { return scenes.emplace_back(new RScene()); }
void Renderer::drop_scene(RScene *scene) noexcept {
    scenes.erase(std::remove(scenes.begin(), scenes.end(), scene), scenes.end());
    delete scene;
}

void Renderer::clear() {
    renderer_thread_process();
    // todo: 我们无法确认在清空这些资源时，是否会有其他的对象还持有引用
    for (auto scene : scenes) {
        delete scene;
    }
}

} // namespace Goonya
