#include "WireFrame.h"

#include "core/cgmath/aabb.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
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
#include <ranges>
#include <span>
#include <vector>

namespace Craft {

struct WireFrameVertex {
    Goonya::Vector3f position;
    Goonya::Vector3f normal; // 实际上是线延伸的方向
};

constexpr std::string_view WIREFRAME_SHADER_NAME = "shaders/craft/wireframe/wire_frame";

WireFrame::WireFrame(Goonya::Graphics::RenderScene &render_scene) {
    assert(Goonya::Graphics::graphics_api); // 不能太早初始化

    const Goonya::Graphics::VertexLayout WIRE_FRAME_LAYOUT =
        Goonya::Graphics::VertexLayoutBuilder()
            .add_attribute(Goonya::Graphics::VertexAttribute::POSITION)
            .add_attribute(Goonya::Graphics::VertexAttribute::NORMAL)
            .build();

    // 创建网格体，其中数据在渲染时通过方块模型动态生成并写入
    mesh = Goonya::Graphics::graphics_api->create_mesh(WIRE_FRAME_LAYOUT);
    mesh->submeshes.resize(1);
    // 只有index_count需要动态改变，另外Topology::LINE因为线宽不能大于1，因此在调试之外就别用了
    mesh->submeshes[0] = Goonya::Graphics::SubMesh{
        .start_index = 0, .index_count = 0, .base_vertex_offset = 0, .topology = Goonya::Graphics::Topology::TRIANGLE};

    Goonya::Graphics::UberShader *shader = Goonya::resources.load_resource<Goonya::Graphics::UberShader>(WIREFRAME_SHADER_NAME).get();
    material = create_ref<Goonya::Graphics::Material>(shader);
    material->set_pipeline_setting("_depth_test",
                                   Goonya::Graphics::PipelineSettingParamType(Goonya::Graphics::DepthTestMode::LESS_EQUAL));
    material->set_pipeline_setting("_cull_mode",
                                   Goonya::Graphics::PipelineSettingParamType(Goonya::Graphics::CullFaceMode::DISABLE));

    mesh_proxy = new Goonya::Graphics::MeshRenderProxy(); // 不需要由WireFrame销毁
    mesh_proxy->mesh = mesh;
    mesh_proxy->materials.emplace_back(material);
    mesh_proxy->aabbs.resize(1);
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{{0, 0, 0}, {0, 0, 0}}; // 不可见

    render_scene.mesh_proxys.emplace(std::unique_ptr<Goonya::Graphics::MeshRenderProxy>{mesh_proxy});
}
void WireFrame::draw_at(Goonya::Vector3f pos, const BakedBlockModel &model) {
    // 现在画线都是用画平面来实现的
    std::vector<WireFrameVertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(48); // 一条线需要4个顶点和6个索引，则方块模型会生成48个顶点和72个索引
    vertices.reserve(72);

    for (const auto &[i, edge] : std::views::enumerate(model.for_all_edges())) {
        const auto &[start, end] = edge;
        Goonya::Vector3f dir = (end - start).normalize();
        vertices.emplace_back(WireFrameVertex{.position = start, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = start, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = end, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = end, .normal = dir});

        indices.emplace_back(i * 4 + 0);
        indices.emplace_back(i * 4 + 1);
        indices.emplace_back(i * 4 + 2);
        indices.emplace_back(i * 4 + 3);
        indices.emplace_back(i * 4 + 2);
        indices.emplace_back(i * 4 + 1);

    }

    mesh->set_vertices(0, std::as_bytes(std::span(vertices)));
    mesh->set_indices(std::span(indices));
    mesh->submeshes[0].index_count = indices.size();

    // 不需要normal_matrix
    mesh_proxy->model_matrix = Goonya::Matrix4::identity().translate(pos);
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{pos, pos + 1};
}
} // namespace Craft