#include "WireFrame.h"

#include "core/cgmath/aabb.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RenderScene.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/UberShader.h"
#include "resource/ResMng.h"

#include <cassert>
#include <cstdint>
#include <span>

namespace Craft {

constexpr Goonya::Vector3f VERTEX_POSITION[]{
    {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1},
};

constexpr uint32_t INDEX[]{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6,
                           7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7

};

constexpr std::string_view WIREFRAME_SHADER_NAME = "shaders/craft/wireframe/wire_frame";

WireFrame::WireFrame(Goonya::Graphics::RenderScene &render_scene) {
    assert(Goonya::Graphics::graphics_api); // 不能太早初始化

    const Goonya::Graphics::VertexLayout WIRE_FRAME_LAYOUT =
        Goonya::Graphics::VertexLayoutBuilder().add_attribute(Goonya::Graphics::VertexAttribute::POSITION).build();

    // 创建网格体
    mesh = Goonya::Graphics::graphics_api->create_mesh(WIRE_FRAME_LAYOUT);

    mesh->set_vertices(0, std::as_bytes(std::span(VERTEX_POSITION)));
    mesh->set_indices(std::span(INDEX));
    mesh->submeshes.emplace_back(Goonya::Graphics::SubMesh{.start_index = 0,
                                                           .index_count = std::extent_v<decltype(INDEX)>,
                                                           .base_vertex_offset = 0,
                                                           .topology = Goonya::Graphics::Topology::LINE});

    Goonya::Graphics::UberShader *shader = Goonya::resources.shader_lib->query_uber_shader(WIREFRAME_SHADER_NAME);
    material = create_ref<Goonya::Graphics::Material>(shader);
    material->set_pipeline_setting(
        "_depth_test", Goonya::Graphics::PipelineSettingParamType(Goonya::Graphics::DepthTestMode::LESS_EQUAL));
    material->set_pipeline_setting("_cull_mode",
                                   Goonya::Graphics::PipelineSettingParamType(Goonya::Graphics::CullFaceMode::BACK));

    mesh_proxy = new Goonya::Graphics::MeshRenderProxy(); // 不需要由WireFrame销毁
    mesh_proxy->mesh = mesh;
    mesh_proxy->materials.emplace_back(material);
    mesh_proxy->aabbs.reserve(mesh->submeshes.size());
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{{0, 0, 0}, {0, 0, 0}}; // 不可见

    render_scene.mesh_proxys.emplace(std::unique_ptr<Goonya::Graphics::MeshRenderProxy>{mesh_proxy});
}
void WireFrame::draw_at(Goonya::Vector3f pos) {
    // 不需要normal_matrix
    mesh_proxy->model_matrix = Goonya::Matrix4::identity().translate(pos);
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{pos, pos + 1};
}
} // namespace Craft