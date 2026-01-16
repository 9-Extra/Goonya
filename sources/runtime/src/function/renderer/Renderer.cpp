#include "Renderer.h"

#include "core/ThreadUtils.h"
#include "core/cgmath/cgmath.h"
#include "core/clock/GameClock.h"
#include "core/log/Log.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/GLRenderTarget.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GAssert.h"

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

    auto [w, h] = GL.get_rendertarget_screen()->get_size();
    Ref<GLFrameBuffer> temp_render_target = create_ref<GLFrameBuffer>(std::make_tuple(w, h));
    temp_render_target->set_depth_stencil_renderbuffer(DepthStencilPixelFormat::DEPTH24_STENCIL8);
    temp_render_target->set_color_renderbuffer(0, TextureStorageFormat::RGB_f8);
    GN_ASSERT(temp_render_target->check_status());
    bool is_screen_painted = false;
    for (auto &&camera : camera_set) {
        if (!camera->render_target || camera->scene == nullptr) continue;
        if (camera->render_target->is_screen()) {
            is_screen_painted = true;
        }

        auto [w, h] = camera->render_target->get_size();

        const Viewport viewport{(int32_t)(camera->rect.x * w), (int32_t)(camera->rect.y * h),
                                (int32_t)(camera->rect.z * w), (int32_t)(camera->rect.w * h)};
        GL.set_viewport(viewport);

        if (camera->render_target == GL.get_rendertarget_screen()) {
            // 不直接绘制到屏幕，而是绘制到临时渲染目标
            temp_render_target->bind_draw();
            // GL.get_rendertarget_screen()->bind_draw();
        } else {
            camera->render_target->bind_draw();
        }

        // 清除旧画面，todo: 根据相机的清除参数来清除
        GL.set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
        GL.clear(true, true, true);

        // 寻找包含且最小，接近中心的天空盒
        Skybox *skybox = nullptr;
        float min_distance = std::numeric_limits<float>::infinity();
        for (auto &&s : camera->scene->skyboxs) {
            if (!s.ignore_range && !s.bbox.contains(camera->get_position())) {
                continue;
            }
            float d = s.ignore_range ? std::numeric_limits<float>::max()
                                     : (s.bbox.center() - camera->get_position()).square();
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

        if (skybox) {
            skybox_pass->run(info);
        }
    }

    if (is_screen_painted) {
        // 将临时渲染目标的内容绘制到屏幕，因为在绘制时Y轴是倒的，所以这里需要翻转Y轴
        temp_render_target->blit(GL.get_rendertarget_screen(), 0, 0, w, h, 0, h, w, 0);
    } else {
        LOG_ERROR("没有相机绑定到屏幕！");
    }
}

void Renderer::clear() {
    if (current_thread_type == ThreadType::RENDER) {
        renderer_thread_process();
    }
    // todo: 我们无法确认在清空这些资源时，是否会有其他的对象还持有引用
    camera_set.clear();
    scene_set.clear();

    geometry_pass.reset();
    skybox_pass.reset();
}

} // namespace Goonya
