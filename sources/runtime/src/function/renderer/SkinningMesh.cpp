#include "SkinningMesh.h"
#include "core/clock/GameClock.h"
#include "function/renderer/Renderer.h"

namespace Goonya {

SkinningMesh::SkinningMesh(const Mesh &mesh) : Mesh(mesh) {
    if (vertex_count == 0) {
        return; // 空网格，update时再创建
    }
    if (!has_attribute(VertexAttribute::POSITION) || !has_attribute(VertexAttribute::NORMAL) ||
        !has_attribute(VertexAttribute::TANGENT)) {
        throw RuntimeError("蒙皮网格必须有位置，法线和切线属性");
    }
    if (!mesh.get_skin_data()) {
        throw RuntimeError("蒙皮网格必须有索引和权重");
    }
    GN_ASSERT(layout.vertex_stride[0] == sizeof(SkinMeshVertex)); // caculated_mesh_buffer的结构应该和mesh_buffer相同
    blended_mesh_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinMeshVertex));
    blended_mesh_buffer->copy_from(mesh_buffer, mesh_buffer->get_size());
    vao = GLVertexLayout{layout};
    vao.set_vertice_buffer(0, blended_mesh_buffer, 0, layout.vertex_stride[0]);
    if (color_buffer) {
        vao.set_vertice_buffer(1, color_buffer, 0, layout.vertex_stride[1]);
    }
    vao.set_index_buffer(indices_buffer);
    ignore_bbox();
}

std::span<std::array<Matrix4f, 2>> SkinningMesh::map_pose_matrix(size_t joint_num) {
    if (renderer.skinning_on_gpu) {
        size_t pose_matrix_buffer_size = joint_num * sizeof(std::array<Matrix4f, 2>);
        if (!pose_matrix_buffer || pose_matrix_buffer->get_size() != pose_matrix_buffer_size) {
            pose_matrix_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, pose_matrix_buffer_size);
            pose_matrix_buffer->set_debug_label("姿态矩阵缓存");
        }
        return {(std::array<Matrix4f, 2> *)pose_matrix_buffer->map(BufferMapOption::WRITE_DISCARD), joint_num};
    } else {
        pose_matrix_buffer_cpu.resize(joint_num);
        return pose_matrix_buffer_cpu;
    }
}

void SkinningMesh::unmap_pose_matrix() {
    last_pose_matrix_update_frame = GAME_CLOCK.current_frame();
    if (renderer.skinning_on_gpu) {
        pose_matrix_buffer->unmap();
    } else {
        // nothing
    }
}

} // namespace Goonya