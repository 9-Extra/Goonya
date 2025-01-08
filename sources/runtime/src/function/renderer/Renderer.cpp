#include "Renderer.h"

#include "HardcodeAssets.h"
#include "core/eventbus/eventbus.h"
#include "platform/display/display.h"
#include "resource/GraphicsResourceBuilder.h"
#include "resource/ResourceJsonLoader.h"

namespace Goonya {
namespace Graphics {
Renderer renderer; // global renderer

void init_resource() {
    // 从json加载大部分的资源
    Resource::load_json("../assets/resources.json");
    // 材质资源比较特殊，暂时通过硬编码加载
    {
        // 平滑着色材质
        Vector3f color_while{1.0f, 1.0f, 1.0f};
        resources.add_material("wood_flat", Resource::MaterialBuilder()
                                                .set_pso(Resource::PSOBuilder().set_uber_shader("flat").build())
                                                .add_uniform(2, sizeof(Vector3f), &color_while)
                                                .add_sampler(3, "wood_diffusion")
                                                .build());
    }

    {
        // 单一颜色材质
        Vector3f color_green{0.0f, 1.0f, 0.0f};
        Resource::MaterialDesc green_material_desc =
            Resource::MaterialBuilder()
                .set_pso(Resource::PSOBuilder().set_uber_shader("single_color").build())
                .add_uniform(2, sizeof(Vector3f), color_green.data())
                .build();

        resources.add_material("default", green_material_desc);
    }
    // 部分硬编码的mesh
    resources.add_mesh("default", {}, {});
    resources.add_mesh("plane", Assets::plane_vertices, Assets::plane_indices);

    // 添加天空盒的mesh，因为格式不一样所以单独处理
    {
        GLuint vao_id, ibo_id, vbo_id;
        glGenVertexArrays(1, &vao_id);
        glGenBuffers(1, &ibo_id);
        glGenBuffers(1, &vbo_id);

        glBindVertexArray(vao_id);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, Assets::skybox_cube_vertices.size() * sizeof(Vector3f),
                     Assets::skybox_cube_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Assets::skybox_cube_indices.size() * sizeof(uint16_t),
                     Assets::skybox_cube_indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3f), (void *)0);
        glEnableVertexAttribArray(0);

        checkError();

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        resources.meshes.add("skybox_cube", Mesh{vao_id, (uint32_t)Assets::skybox_cube_indices.size()});
        // 注册销毁用回调函数在程序结束时调用
        resources.deconstructors.emplace_back([vao_id, vbo_id, ibo_id]() {
            glDeleteVertexArrays(1, &vao_id);
            glDeleteBuffers(1, &vbo_id);
            glDeleteBuffers(1, &ibo_id);
            checkError();
        });
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
    pickup_pass = std::make_unique<PickupPass>();
}
} // namespace Graphics
}