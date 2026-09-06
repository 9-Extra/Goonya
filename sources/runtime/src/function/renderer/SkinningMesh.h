#pragma once

#include "core/cgmath/aabb.h"
#include "function/renderer/Mesh.h"
#include <cstddef>
#include <ranges>

namespace Goonya {

class SkinningMesh : public Mesh {
private:
    Ref<GLBuffer> caculated_mesh_buffer; // mesh_buffer是静态数据，caculated_mesh_buffer是实时计算的动态数据

    // 用于cpu计算蒙皮的缓存数据
    std::vector<SkinMeshVertex> mesh_buffer_cpu;

public:
    explicit SkinningMesh(const Mesh &mesh);

    void on_init(const MeshDataArrays &data) override {
        caculated_mesh_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinMeshVertex));
        caculated_mesh_buffer->copy_from(mesh_buffer, 0, mesh_buffer->get_size());
        vao.set_vertice_buffer(0, caculated_mesh_buffer, 0, layout.vertex_stride[0]);
        // caculated_mesh_buffer的值由外部负责更新
        mesh_buffer_cpu.resize(vertex_count);
        // 这里的计算和上传mesh_buffer的那次重复了
        for (size_t i : std::views::iota(size_t(0), vertex_count)) {
            mesh_buffer_cpu[i] = {.position = data.position[i], .normal = data.normal[i], .tangent = data.tangent[i]};
        }
        ignore_bbox();
    }

    void write_skin_data(std::span<const SkinVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        if (!skin_buffer) {
            skin_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinVertex));
        }
        skin_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }
    std::span<const SkinMeshVertex> get_mesh_buffer_data() const noexcept { return mesh_buffer_cpu; }

    Ref<GLBuffer> get_caculated_mesh_buffer() const noexcept { return caculated_mesh_buffer; }

    void write_skinned_vertices(std::span<const SkinMeshVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        GN_ASSERT_MSG(caculated_mesh_buffer, "先update再填充mesh数据");
        caculated_mesh_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }

private:
    void ignore_bbox() {
        for (auto &s : submeshes) {
            s.aabb = BoundingBox::infinite();
        }
    }
};

} // namespace Goonya