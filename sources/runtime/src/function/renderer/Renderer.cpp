#include "Renderer.h"

#include "core/ThreadUtils.h"
#include "core/cgmath/cgmath.h"
#include "core/clock/GameClock.h"
#include "core/log/Log.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/RenderTarget.h"
#include "resource/ResMng.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <ratio>

namespace Goonya::Graphics {
Renderer renderer; // global renderer

void Renderer::init() {
    current_thread_type = ThreadType::RENDER; // 目前先不拆分线程，资源加载的问题没有解决
    Graphics::initialize(Graphics::GraphicsAPIType::OPENGL);

    resources.init(u8"../assets/");

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

        // static Ref<FrameBuffer> test_target;
        // static Ref<Texture> render_texture;
        // if (!test_target){
        //     test_target = graphics_api->create_rendertarget({1024, 1024});
        //     test_target->set_depth_renderbuffer(RenderBufferPixelFormat::DEPTH24_STENCIL8);
        // }
        // if (!render_texture){
        //     TextureCreateDesc desc{TextureType::TEXTURE_2D, TextureStorageFormat::RGBA_f32, {1024, 1024, 0}};
        //     render_texture = graphics_api->create_texture(desc);
        //     test_target->attach_color_texture(0, render_texture);
        // }
        // camera->render_target = test_target;
        // camera->render_target->check_status();

        // todo: ViewPort的大小计算不完善
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

        // FIBITMAP *image = render_texture->export_image();
        // assert(image);

        // static int i = 0;
        // if (!FreeImage_Save(FIF_PNG, image, std::format("output{}.png", i).c_str())) {
        //     LOG_ERROR("导出图像失败");
        // }
        // i++;
        // EventBus::dispatch_event(Events::EngineStop{});
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

    Graphics::drop();
}

} // namespace Goonya::Graphics
