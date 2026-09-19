#pragma once

#include "core/clock/GameClock.h"
#include "function/renderer/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include <cstddef>
#include <ranges>

namespace Goonya {

class SkinningMesh : public Mesh {
private:
    Ref<GLBuffer> blended_mesh_buffer; // mesh_buffer是静态数据，blended_mesh_buffer是实时计算的动态数据
    Ref<GLBuffer> pose_matrix_buffer;  // gpu绑定矩阵

    // 用于cpu计算蒙皮的数据
    std::vector<SkinMeshVertex> mesh_buffer_cpu;
    std::vector<std::array<Matrix4f, 2>> pose_matrix_buffer_cpu;

    GameClock::FrameCounter last_pose_matrix_update_frame = 0;
    GameClock::FrameCounter last_mesh_update_frame = 0; // 在Pipeline中用于判断是否需要更新

public:
    friend class Pipeline;
    explicit SkinningMesh(const Mesh &mesh);

    void on_init(const MeshDataArrays &data) override {
        blended_mesh_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinMeshVertex));
        blended_mesh_buffer->copy_from(mesh_buffer, 0, mesh_buffer->get_size());
        vao.set_vertice_buffer(0, blended_mesh_buffer, 0, layout.vertex_stride[0]);
        // caculated_mesh_buffer的值由外部负责更新
        mesh_buffer_cpu.resize(vertex_count);
        // 这里的计算和上传mesh_buffer的那次重复了
        for (size_t i : std::views::iota(size_t(0), vertex_count)) {
            mesh_buffer_cpu[i] = {.position = data.position[i], .normal = data.normal[i], .tangent = data.tangent[i]};
        }
        ignore_bbox();
    }

    std::span<std::array<Matrix4f, 2>> map_pose_matrix(size_t joint_num);
    void unmap_pose_matrix();

    void write_skin_data(std::span<const SkinVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        if (!skin_buffer) {
            skin_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinVertex));
        }
        skin_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }

    std::span<const SkinMeshVertex> get_mesh_buffer_data() noexcept {
        // 尚不能保证mesh_buffer_cpu和mesh_buffer数据完全同步，小心使用
        if (!mesh_buffer) {
            return {};
        }
        if (mesh_buffer_cpu.empty() && mesh_buffer->get_size() != 0) {
            mesh_buffer_cpu.resize(vertex_count);
            mesh_buffer->read((std::byte *)mesh_buffer_cpu.data(), mesh_buffer->get_size()); // 目前只能靠回读了
        }
        return mesh_buffer_cpu;
    }

    Ref<GLBuffer> get_blended_mesh_buffer() const noexcept { return blended_mesh_buffer; }

    void write_skinned_vertices(std::span<const SkinMeshVertex> src) {
        GN_ASSERT(src.size() == vertex_count);
        GN_ASSERT_MSG(blended_mesh_buffer, "先update再填充mesh数据");
        blended_mesh_buffer->write(std::as_bytes(src), BufferMapOption::WRITE_DISCARD);
    }

private:
    void ignore_bbox() {
        // 尽管会导致偶尔错误的剔除，还是暂时不忽略包围盒子
        /*
        for (auto &s : submeshes) {
            s.aabb = BoundingBox::infinite();
        }
        */
    }
};

} // namespace Goonya