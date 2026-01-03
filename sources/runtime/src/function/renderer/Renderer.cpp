#include "Renderer.h"

#include "core/cgmath/cgmath.h"
#include "core/clock/GameClock.h"
#include "core/log/Log.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Graphics.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <ratio>

namespace Goonya {
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
        GL.set_viewport(viewport);
        camera->render_target->bind_draw();

        // 清除旧画面
        GL.set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
        GL.clear(true, true, true);

        // 寻找包含且最小，接近中心的天空盒
        Skybox* skybox = nullptr;
        float min_distance = std::numeric_limits<float>::infinity();
        for (auto&& s : camera->scene->skyboxs) {
            if (!s.ignore_range && !s.bbox.contains(camera->get_position())) {
                continue;
            }
            float d = s.ignore_range ? std::numeric_limits<float>::max() : (s.bbox.center() - camera->get_position()).square();
            if (d < min_distance) {
                skybox = &s;
                min_distance = d;
            }
        }
            
        PassRenderInfo info{
            .camera = camera.get(),
            .viewport = viewport,
            .screen_size = {(float)viewport.width, (float)viewport.height},
            .time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(GAME_CLOCK.total()).count(),
        };

        if (skybox) {
            info.env_map = skybox->env_map;
            info.skybox_material = skybox->skybox_material;
        }

        geometry_pass->run(info);
        skybox_pass->run(info);

    }

    if (!is_screen_painted) {
        LOG_ERROR("没有相机绑定到屏幕！");
    }
}

void Renderer::clear() {
    renderer_thread_process();
    // todo: 我们无法确认在清空这些资源时，是否会有其他的对象还持有引用
    camera_set.clear();
    scene_set.clear();

    geometry_pass.reset();
    skybox_pass.reset();
}

} // namespace Goonya
