#include "WireFrame.h"

#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "craft/block/block_model.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RenderScene.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/UberShader.h"
#include "resource/ResMng.h"


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

WireFrame::WireFrame(Goonya::RenderScene &render_scene) {
    // 确保OpenGL上下文已初始化，避免过早创建资源导致错误
    GN_ASSERT(Goonya::GL.is_initialized()); // 不能太早初始化

    // 构建线框渲染专用的顶点布局，包含位置(position)和法线(normal)两个属性
    // 这里的法线实际上用于存储线的延伸方向，用于后续的几何着色器处理
    const Goonya::VertexLayout WIRE_FRAME_LAYOUT =
        Goonya::VertexLayoutBuilder()
            .add_attribute(Goonya::VertexAttribute::POSITION)  // 顶点位置信息
            .add_attribute(Goonya::VertexAttribute::NORMAL)    // 法线信息(实际存储线方向)
            .build();

    // 创建网格体对象，其中顶点数据会在渲染时动态生成并写入
    mesh = create_ref<Goonya::GLMesh>(WIRE_FRAME_LAYOUT);
    mesh->submeshes.resize(1);
    // 只有index_count需要动态改变，另外Topology::LINE因为线宽不能大于1，因此在调试之外就别用了
    // 使用三角形拓扑结构，通过几何着色器将三角形转换为线条，以实现更宽的线宽效果
    mesh->submeshes[0] = Goonya::SubMesh{
        .start_index = 0, .index_count = 0, .base_vertex_offset = 0, .topology = Goonya::Topology::TRIANGLE};

    // 从资源管理器加载线框着色器，该着色器负责将三角形转换为线条
    Goonya::UberShader *shader = Goonya::resources.load_resource<Goonya::UberShader>(WIREFRAME_SHADER_NAME).get();
    material = create_ref<Goonya::Material>(shader);

    // 创建网格渲染代理对象，用于管理网格和材质的渲染
    mesh_proxy = new Goonya::MeshRenderProxy(); // 不需要由WireFrame销毁
    mesh_proxy->mesh = mesh;
    mesh_proxy->materials.emplace_back(material);
    mesh_proxy->aabbs.resize(1);
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{{0, 0, 0}, {0, 0, 0}}; // 初始化为不可见的边界框

    // 将网格代理添加到渲染场景中，使其参与渲染流程
    render_scene.mesh_proxys.emplace(std::unique_ptr<Goonya::MeshRenderProxy>{mesh_proxy});
}
void WireFrame::draw_at(Goonya::Vector3f pos, const BakedBlockModel &model) {
    // 现代OpenGL渲染中，线条是通过渲染两个三角形来模拟的
    // 这种方法可以实现更宽的线宽效果，而直接使用LINE拓扑结构受限于线宽不能超过1的限制
    std::vector<WireFrameVertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(48); // 每条边需要4个顶点和6个索引，方块模型最多有12条边，总计48个顶点和72个索引
    indices.reserve(72);

    // 遍历方块模型的所有边，为每条边生成两个三角形
    for (const auto &[i, edge] : std::views::enumerate(model.for_all_edges())) {
        const auto &[start, end] = edge;
        Goonya::Vector3f dir = (end - start).normalize();  // 计算边的方向向量，用于几何着色器
        
        // 为每条边创建4个顶点，形成两个三角形的顶点布局
        // 顶点布局：0-1-2 和 2-3-1 形成两个三角形
        vertices.emplace_back(WireFrameVertex{.position = start, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = start, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = end, .normal = dir});
        vertices.emplace_back(WireFrameVertex{.position = end, .normal = dir});

        // 生成索引数据，定义两个三角形的顶点连接关系
        indices.emplace_back(i * 4 + 0);
        indices.emplace_back(i * 4 + 1);
        indices.emplace_back(i * 4 + 2);
        indices.emplace_back(i * 4 + 3);
        indices.emplace_back(i * 4 + 2);
        indices.emplace_back(i * 4 + 1);

    }

    // 将生成的顶点数据上传到GPU
    mesh->set_vertices(0, std::as_bytes(std::span(vertices)));
    mesh->set_indices(std::span(indices));
    mesh->submeshes[0].index_count = indices.size();

    // 设置模型变换矩阵，将线框从局部坐标系转换到世界坐标系
    // 不需要normal_matrix，因为线框渲染不依赖于法线变换
    mesh_proxy->model_matrix = Goonya::Matrix4f::identity().translate(pos);
    mesh_proxy->aabbs[0] = Goonya::BoundingBox{pos, pos + 1};  // 更新边界框为1x1x1的立方体
}
} // namespace Craft