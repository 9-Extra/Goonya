#include "Renderer.h"

#include "HardcodeAssets.h"
#include "core/Bytes.h"
#include "core/ThreadType.h"
#include "core/cgmath.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/log/Log.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/RenderTarget.h"
#include "resource/Resource.h"
#include "resource/ResourceJsonLoader.h"
#include <FreeImage.h>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>

namespace Goonya::Graphics {
Renderer renderer; // global renderer

void init_resource() {
    // 部分硬编码的mesh
    MeshDesc plane{Assets::plane_vertices_vertex_layout, Bytes::from_span(std::span(Assets::plane_vertices)),
                   Assets::plane_indices, Topology::TRIANGLE};
    Resource::resources.meshes.add("plane", std::move(plane));
    // 添加天空盒的mesh，因为格式不一样所以单独处理
    MeshDesc skybox_cube{Assets::skybox_cube_vertex_layout, Bytes::from_span(std::span(Assets::skybox_cube_vertices)),
                         Assets::skybox_cube_indices, Topology::TRIANGLE};
    Resource::resources.meshes.add("skybox_cube", std::move(skybox_cube));
    // 从json加载大部分的资源
    Resource::load_json("../assets/resources.json");
}
void Renderer::init() {
    current_thread_type = ThreadType::RENDER; // 目前先不拆分线程，资源加载的问题没有解决
    Graphics::initialize(Graphics::GraphicsAPIType::OPENGL);

    Resource::resources.init();
    init_resource();

    lambertian_pass = std::make_unique<LambertianPass>();
    skybox_pass = std::make_unique<SkyBoxPass>();
}

void Renderer::run_all_tasks() {
    // 运行所有推入的Task
    while (!render_tasks.empty()) {
        std::invoke(std::move(render_tasks.front()));
        render_tasks.pop();
    }
}

void Renderer::render() {
    static size_t frame = 0;
    graphics_api->push_debug_group_label(std::format("Frame {}", frame++));

    run_all_tasks();

    bool is_screen_painted = false;
    for (CameraRenderProxy *camera : cameras) {
        if (!camera->render_target)
            continue;
        if (camera->render_target->is_screen()) {
            is_screen_painted = true;
        }

        // static intrusive_ptr<FrameBuffer> test_target;
        // static intrusive_ptr<Texture> render_texture;
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
        camera->view_port = {0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h)};

        current_camera = camera;
        current_camera->render_target->bind_draw();
        graphics_api->set_viewport(current_camera->view_port);

        // 清除旧画面
        graphics_api->set_clear_parameter(Color{0.0f, 0.0f, 0.0f, 1.0f});
        graphics_api->clear(true, true, true);

        lambertian_pass->run();
        skybox_pass->run();
        // pickup_pass->run();

        lambertian_pass->reset();
        pointlights.clear();

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

    graphics_api->pop_debug_group_label();
}

void Renderer::clear() {
    run_all_tasks();

    assert(meshes.empty());

    current_skyboxs.clear();

    lambertian_pass.reset();
    skybox_pass.reset();

    Graphics::drop();
}

} // namespace Goonya::Graphics
