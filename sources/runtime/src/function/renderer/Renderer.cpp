#include "Renderer.h"

#include "HardcodeAssets.h"
#include "core/cgmath.h"
#include "core/eventbus/eventbus.h"
#include "core/metatype/metatype.h"
#include "function/renderer/RenderResource.h"
#include "platform/display/display.h"
#include "resource/GraphicsResourceBuilder.h"
#include "resource/ResourceJsonLoader.h"
#include "resource/resources.h"

namespace Goonya {
namespace Graphics {
Renderer renderer; // global renderer

void init_resource() {
    // 从json加载大部分的资源
    Resource::load_json("../assets/resources.json");
    // 通过硬编码加载的部分资源
    {
        // 平滑着色材质
        Vector3f color_while{1.0f, 1.0f, 1.0f};
        resources.add_material("wood_flat", Resource::MaterialBuilder()
                                                .set_pso(Resource::PSOBuilder().set_uber_shader("flat").build())
                                                .add_parameter("base_color", color_while)
                                                .add_sampler("color_texture", "wood_diffusion")
                                                .build());
    }

    {
        // 单一颜色材质
        Vector3f color_green{0.0f, 1.0f, 0.0f};
        Resource::MaterialDesc green_material_desc =
            Resource::MaterialBuilder()
                .set_pso(Resource::PSOBuilder().set_uber_shader("single_color").build())
                .add_parameter("color", color_green)
                .build();

        resources.add_material("default", green_material_desc);
    }
    {
        // 部分硬编码的mesh
        resources.add_mesh("default", Resource::VertexLayout{{}, 0}, {}, {});

        const Resource::VertexLayout vertex_layout{
            {{Resource::VertexAttribute::POSITION, Meta::FieldType::vec3f, offsetof(Graphics::Vertex, position)},
             {Resource::VertexAttribute::NORMAL, Meta::FieldType::vec3f, offsetof(Graphics::Vertex, normal)},
             {Resource::VertexAttribute::TANGENT, Meta::FieldType::vec3f, offsetof(Graphics::Vertex, tangent)},
             {Resource::VertexAttribute::UV, Meta::FieldType::vec2f, offsetof(Graphics::Vertex, uv)}},
            sizeof(Graphics::Vertex)};

        resources.add_mesh("plane", vertex_layout, std::span(Assets::plane_vertices), Assets::plane_indices);
    }

    {
        // 添加天空盒的mesh，因为格式不一样所以单独处理
        const Resource::VertexLayout vertex_layout{{{Resource::VertexAttribute::POSITION, Meta::FieldType::vec3f, 0}},
                                                   sizeof(Vector3f)};
        resources.add_mesh("skybox_cube", vertex_layout, std::span(Assets::skybox_cube_vertices),
                           std::span(Assets::skybox_cube_indices));
    }
}
void Renderer::init() {
    init_resource();

    auto [w, h] = Display::get_size();
    LOG_INFO("初始Framebuffer大小{}x{}", w, h);
    renderer.set_viewport(0, 0, w, h); // 初始化时也需要设置一下视口

    EventBus::subscribe_event<Display::Events::SysDisplayResize, void>(
        0, nullptr, [](void *, Display::Events::SysDisplayResize &e) {
            auto [w, h] = e.size;
            renderer.set_viewport(0, 0, w, h);
            return false;
        });

    lambertian_pass = std::make_unique<LambertianPass>();
    skybox_pass = std::make_unique<SkyBoxPass>();
}

void Renderer::render() {
    // 清除旧画面
    graphics_api->bind_rendertarget_screen();
    graphics_api->set_clear_parameter(Color{0.0f, 0.0f, 0.0f});
    graphics_api->clear();
    debug_check_error();

    if (active_camera == nullptr) {
        LOG_WARN("主相机未设置");
        return;
    }

    lambertian_pass->run();
    skybox_pass->run();
    // pickup_pass->run();

    lambertian_pass->reset();
    pointlights.clear();
    debug_check_error();
}
} // namespace Graphics
} // namespace Goonya