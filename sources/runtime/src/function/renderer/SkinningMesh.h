#pragma once

#include "function/renderer/Mesh.h"

namespace Goonya {

class SkinningMesh : public Mesh {
private:
    Ref<GLBuffer> caculated_mesh_buffer; // mesh_buffer是静态数据，caculated_mesh_buffer是实时计算的动态数据
    Ref<GLBuffer> skin_buffer;           // vec4i joints, vec4f weight，用于GPU侧蒙皮，不属于顶点数据
public:
    explicit SkinningMesh(const Mesh &mesh);

    void on_reconstruct() override {
        caculated_mesh_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinMeshVertex));
        caculated_mesh_buffer->copy_from(mesh_buffer, 0, mesh_buffer->get_size());
        vao.set_vertice_buffer(0, caculated_mesh_buffer, 0, layout.vertex_stride[0]);
        // caculated_mesh_buffer的值由外部负责更新
    }

    void write_skin_data(std::span<const SkinVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        if (!skin_buffer) {
            skin_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinVertex));
        }
        skin_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }

    Ref<GLBuffer> get_caculated_mesh_buffer() const noexcept { return caculated_mesh_buffer; }

    void write_skinned_vertices(std::span<const SkinMeshVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        GN_ASSERT_MSG(caculated_mesh_buffer, "先update再填充mesh数据");
        caculated_mesh_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }
};

} // namespace Goonya