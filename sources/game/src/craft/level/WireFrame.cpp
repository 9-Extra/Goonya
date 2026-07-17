#include "WireFrame.h"

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
#include "function/renderer/Material.h"
#include "function/renderer/RScene.h"
#include "function/renderer/UberShader.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"

#include <cstdint>
#include <ranges>
#include <vector>

namespace Craft {

constexpr std::string_view WIREFRAME_SHADER_NAME = "shaders/craft/wireframe/wire_frame";

WireFrame::WireFrame(Goonya::RScene *scene) : renderable(*scene) {
    // 确保OpenGL上下文已初始化，避免过早创建资源导致错误
    GN_ASSERT(Goonya::GL.is_initialized()); // 不能太早初始化

    mesh = Goonya::MeshBuilder::build_empty();
    // 只有index_count需要动态改变，另外Topology::LINE因为线宽不能大于1，因此在调试之外就别用了
    // 使用三角形拓扑结构，通过几何着色器将三角形转换为线条，以实现更宽的线宽效果
    builder.submeshes = {Goonya::SubMesh{
        .start_index = 0,
        .index_count = 0,
        .base_vertex_offset = 0,
        .topology = Goonya::Topology::TRIANGLE,
        .aabb = Goonya::BoundingBox{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    }};

    // 从资源管理器加载线框着色器，该着色器负责将三角形转换为线条
    Goonya::UberShader *shader = Goonya::resources.load_resource<Goonya::UberShader>(WIREFRAME_SHADER_NAME).get();
    materials = {create_ref<Goonya::Material>(shader)};

    renderable.set_mesh(mesh);
    renderable.set_materials(materials);
    renderable.set_hidden(true); // 初始不可见
}
void WireFrame::draw_at(Goonya::Vector3f pos, const BakedBlockModel &model) {
    // 现代OpenGL渲染中，线条是通过渲染两个三角形来模拟的
    // 这种方法可以实现更宽的线宽效果，而直接使用LINE拓扑结构受限于线宽不能超过1的限制
    // 清空上一次的数据，保留capacity
    builder.position.clear();
    builder.normal.clear();
    builder.indices.clear();
    builder.submeshes.clear();
    // 每条边需要4个顶点和6个索引，方块模型最多有12条边，总计48个顶点和72个索引
    builder.position.reserve(48);
    builder.normal.reserve(48);
    builder.indices.reserve(72);

    // 遍历方块模型的所有边，为每条边生成两个三角形
    for (const auto &[i, edge] : std::views::enumerate(model.for_all_edges())) {
        const auto &[start, end] = edge;
        Goonya::Vector3f dir = (end - start).normalize(); // 计算边的方向向量

        // 为每条边创建4个顶点，形成两个三角形的顶点布局
        // 顶点布局：0-1-2 和 2-3-1 形成两个三角形
        builder.position.push_back(start);
        builder.position.push_back(start);
        builder.position.push_back(end);
        builder.position.push_back(end);

        builder.normal.push_back(dir);
        builder.normal.push_back(dir);
        builder.normal.push_back(dir);
        builder.normal.push_back(dir);

        // 生成索引数据，定义两个三角形的顶点连接关系
        const uint32_t o_i = (uint32_t)i * 4;
        builder.indices.push_back(o_i + 0);
        builder.indices.push_back(o_i + 1);
        builder.indices.push_back(o_i + 2);
        builder.indices.push_back(o_i + 3);
        builder.indices.push_back(o_i + 2);
        builder.indices.push_back(o_i + 1);
    }

    builder.submeshes = {Goonya::SubMesh{
        .start_index = 0,
        .index_count = (uint32_t)builder.indices.size(),
        .base_vertex_offset = 0,
        .topology = Goonya::Topology::TRIANGLE,
        .aabb = Goonya::BoundingBox{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
    }};

    mesh = builder.build();
    renderable.set_mesh(mesh);
    renderable.set_transform(Goonya::Matrix4f::identity().translate(pos));
    renderable.set_hidden(false); // 可见
}
} // namespace Craft