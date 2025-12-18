#include "Renderer.h"

#include "core/cgmath/cgmath.h"
#include "core/clock/GameClock.h"
#include "core/log/Log.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/RenderTarget.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <ratio>

namespace Goonya::Graphics {
Renderer renderer; // global renderer

void Renderer::init() {
    geometry_pass = std::make_unique<GeometryPass>();
    skybox_pass = std::make_unique<SkyBoxPass>();
}

void Renderer::render() {

    renderer_thread_process();

    bool is_screen_painted = false;
    for (auto &&camera : camera_set) {
        if (!camera->render_target || camera->scene == nullptr)
            continue;
        if (camera->render_target->is_screen()) {
            is_screen_painted = true;
        }

        auto [w, h] = camera->render_target->get_size();
        const Viewport viewport{(int32_t)(camera->rect.x * w), (int32_t)(camera->rect.y * h),
                                (int32_t)(camera->rect.z * w), (int32_t)(camera->rect.w * h)};
        graphics_api->set_viewport(viewport);
        camera->render_target->bind_draw();

        // 清除旧画面
        graphics_api->set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
        graphics_api->clear(true, true, true);
        
        PassRenderInfo info{
            .camera = camera.get(),
            .viewport = viewport,
            .screen_size = {(float)viewport.width, (float)viewport.height},
            .time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GAME_CLOCK.total()).count(),
        };

        geometry_pass->run(info);
        skybox_pass->run(info);

    }

    if (!is_screen_painted) {
        LOG_ERROR("没有相机绑定到屏幕！");
    }
}

void Renderer::clear() {
    renderer_thread_process();

    camera_set.clear();
    scene_set.clear();

    geometry_pass.reset();
    skybox_pass.reset();
}

} // namespace Goonya::Graphics
