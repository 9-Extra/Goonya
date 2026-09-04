#include "Mesh.h"

#include <cstddef>
#include <cstdint>

namespace Goonya {

void Mesh::reconstruct(const MeshDataArrays &data) {
    layout = {};
    if (!data.position.empty()) add_attribute(layout, VertexAttribute::POSITION);
    if (!data.normal.empty()) add_attribute(layout, VertexAttribute::NORMAL);
    if (!data.tangent.empty()) add_attribute(layout, VertexAttribute::TANGENT);
    if (!data.color.empty()) add_attribute(layout, VertexAttribute::COLOR);
    if (!data.uv.empty()) add_attribute(layout, VertexAttribute::UV);
    vao = GLVertexLayout{layout}; // 初始化vao

    GN_ASSERT(!data.position.empty());
    vertex_count = data.position.size();

#if 0
    // 规模一致性校验
    GN_ASSERT(normal.empty() || normal.size() == vertex_count);
    GN_ASSERT(tangent.empty() || tangent.size() == vertex_count);
    GN_ASSERT(uv.empty() || uv.size() == vertex_count);
    GN_ASSERT(color.empty() || color.size() == vertex_count);
    GN_ASSERT(std::all_of(indices.begin(), indices.end(), [vertex_count](uint32_t i) { return i < vertex_count; }));
#endif

    // 构建mesh_buffer (stream 0: position + normal + tangent)
    uint32_t mesh_stride = layout.vertex_stride[0];
    if (mesh_stride > 0) {
        std::vector<std::byte> mesh_data;
        mesh_data.resize(mesh_stride * vertex_count);
        for (size_t i = 0; i < vertex_count; ++i) {
            size_t base = i * mesh_stride;
            if (!data.position.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::POSITION].offset;
                std::memcpy(mesh_data.data() + base + offset, &data.position[i], sizeof(Vector3f));
            }
            if (!data.normal.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::NORMAL].offset;
                std::memcpy(mesh_data.data() + base + offset, &data.normal[i], sizeof(Vector3f));
            }
            if (!data.tangent.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::TANGENT].offset;
                std::memcpy(mesh_data.data() + base + offset, &data.tangent[i], sizeof(Vector4f));
            }
        }
        mesh_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::span(mesh_data));
        vao.set_vertice_buffer(0, mesh_buffer, 0, mesh_stride);
    }

    // 构建color_buffer (stream 1: color + uv)
    uint32_t color_stride = layout.vertex_stride[1];
    if (color_stride > 0) {
        std::vector<std::byte> color_data;
        color_data.resize(color_stride * vertex_count);
        for (size_t i = 0; i < vertex_count; ++i) {
            size_t base = i * color_stride;
            if (!data.color.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::COLOR].offset;
                std::memcpy(color_data.data() + base + offset, &data.color[i], sizeof(Vector3f));
            }
            if (!data.uv.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::UV].offset;
                std::memcpy(color_data.data() + base + offset, &data.uv[i], sizeof(Vector2f));
            }
        }
        color_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::span(color_data));
        vao.set_vertice_buffer(1, color_buffer, 0, color_stride);
    }

    if (!data.indices.empty()) {
        set_indices_data(data.indices);
    }

    if (data.submeshes) {
        submeshes = data.submeshes.value();
    }

    on_reconstruct();
}

} // namespace Goonya