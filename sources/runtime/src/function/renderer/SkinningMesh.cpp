#include "SkinningMesh.h"

namespace Goonya {

SkinningMesh::SkinningMesh(const Mesh &mesh) : Mesh(mesh) {
    if (vertex_count == 0) {
        return; // 空网格，update时再创建
    }
    if (!has_attribute(VertexAttribute::POSITION) || !has_attribute(VertexAttribute::NORMAL) ||
        !has_attribute(VertexAttribute::TANGENT)) {
        throw RuntimeError("蒙皮网格必须有位置，法线和切线属性");
    }
    GN_ASSERT(layout.vertex_stride[0] == sizeof(SkinMeshVertex)); // caculated_mesh_buffer的结构应该和mesh_buffer相同
    caculated_mesh_buffer = create_ref<GLBuffer>(BufferType::MODIFIABLE, vertex_count * sizeof(SkinMeshVertex));
    vao = GLVertexLayout{layout};
    vao.set_vertice_buffer(0, caculated_mesh_buffer, 0, layout.vertex_stride[0]);
    if (color_buffer) {
        vao.set_vertice_buffer(1, color_buffer, 0, layout.vertex_stride[1]);
    }
    vao.set_index_buffer(indices_buffer);
}
} // namespace Goonya