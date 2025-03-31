#include "Renderer.h"

#include "HardcodeAssets.h"
#include "core/cgmath.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/log/Log.h"
#include "function/renderer/RenderAspect.h"
#include "function/renderer/RenderResource.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/graphics.h"
#include "resource/ResourceJsonLoader.h"
#include <FreeImage.h>
#include <cstdint>

namespace Goonya {
namespace Graphics {
Renderer renderer; // global renderer

void init_resource() {
    // 部分硬编码的mesh
    resources.add_mesh("default", VertexLayout{{}, 0}, {}, {});
    resources.add_mesh("plane", Assets::plane_vertices_vertex_layout, std::span(Assets::plane_vertices),
                       Assets::plane_indices);
    // 添加天空盒的mesh，因为格式不一样所以单独处理
    resources.add_mesh("skybox_cube", Assets::skybox_cube_vertex_layout, std::span(Assets::skybox_cube_vertices),
                       std::span(Assets::skybox_cube_indices));
    // 从json加载大部分的资源
    Resource::load_json("../assets/resources.json");
    // 通过硬编码加载的部分资源
}
void Renderer::init() {
    resources.init();
    init_resource();

    lambertian_pass = std::make_unique<LambertianPass>();
    skybox_pass = std::make_unique<SkyBoxPass>();
}

void Renderer::render() {
    bool is_screen_painted = false;
    for (CameraRenderInfo *camera : cameras) {
        if (!camera->render_target)
            continue;
        if (camera->render_target->is_screen()){
            is_screen_painted = true;
        }

        // intrusive_ptr<FrameBuffer> test_target;
        // intrusive_ptr<Texture> render_texture;
        // test_target = graphics_api->create_rendertarget({1024, 1024});
        // TextureCreateDesc desc{TextureType::TEXTURE_2D, TextureStorageFormat::RGBA_f32, {1024, 1024, 0}};
        // render_texture = graphics_api->create_texture(desc);
        // test_target->attach_color_texture(0, render_texture);
        // test_target->set_depth_renderbuffer(RenderBufferPixelFormat::DEPTH24_STENCIL8);
        // camera->render_target = test_target;
        // camera->render_target->check_status();

        // todo: ViewPort的大小计算不完善
        auto [w, h] = camera->render_target->get_size();
        camera->view_port = {0, 0, (int32_t)w, (int32_t)h};
    
        current_camera = camera;
        current_camera->render_target->bind_draw();
        graphics_api->set_viewport(current_camera->view_port);
    
        // 清除旧画面
        graphics_api->set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
        graphics_api->clear();
        debug_check_error();

        lambertian_pass->run();
        skybox_pass->run();
        // pickup_pass->run();

        lambertian_pass->reset();
        pointlights.clear();

        // FIBITMAP *image = render_texture->read_image();
        // assert(image);

        // if (!FreeImage_Save(FIF_PNG, image, "output.png")) {
        //     LOG_ERROR("导出图像失败");
        // }
        // EventBus::dispatch_event(Events::EngineStop{});

        debug_check_error();
    }

    if (!is_screen_painted){
        LOG_ERROR("没有相机绑定到屏幕！");
    }
}
} // namespace Graphics
} // namespace Goonya