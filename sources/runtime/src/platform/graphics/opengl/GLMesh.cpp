#include "GLMesh.h"
#include "platform/graphics/Mesh.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Goonya {
namespace Graphics {

static GLenum Topology2OpenGL(Topology t) noexcept {
    switch (t) {
    case Topology::POINT:
        return GL_POINTS;
    case Topology::LINE:
        return GL_LINES;
    case Topology::TRIANGLE:
        return GL_TRIANGLES;
    }
    return GL_INVALID_VALUE;
}

static std::tuple<GLuint, GLenum> FieldType2OpenGLComponentsAndType(Meta::FieldType type) {
    switch (type) {

    case Meta::FieldType::nul:
        break;
    case Meta::FieldType::i32:
        return {1, GL_INT};
    case Meta::FieldType::i64:
        throw RuntimeError("Invaild Type for Vertex Array");
    case Meta::FieldType::u32:
        return {1, GL_UNSIGNED_INT};
    case Meta::FieldType::u64:
        throw RuntimeError("Invaild Type for Vertex Array");
    case Meta::FieldType::f32:
        return {1, GL_FLOAT};
    case Meta::FieldType::f64:
        return {1, GL_DOUBLE};
    case Meta::FieldType::vec2f:
        return {2, GL_FLOAT};
    case Meta::FieldType::vec3f:
        return {3, GL_FLOAT};
    case Meta::FieldType::vec4f:
        return {4, GL_FLOAT};
    case Meta::FieldType::mat4f:
        throw RuntimeError("Invaild Type for Vertex Array");
    }

    throw RuntimeError("Invaild Field Type");
}

GLMesh::GLMesh(Topology topology, Resource::VertexLayout layout, intrusive_ptr<GLBuffer> vertex_buffers,
               intrusive_ptr<GLBuffer> index_buffer)
    : GLMesh(std::vector<SubMesh>{SubMesh{0, (uint32_t)index_buffer->get_size(), topology}}, layout, vertex_buffers,
             index_buffer) {}

GLMesh::GLMesh(const std::vector<SubMesh> &submeshes, Resource::VertexLayout layout,
               intrusive_ptr<GLBuffer> vertex_buffers, intrusive_ptr<GLBuffer> index_buffer)
    : layout(std::move(layout)), vertex_buffer(vertex_buffers), index_buffer(index_buffer) {
    // 使用顶点格式创建VAO
    glCreateVertexArrays(1, &vao_id);
    GLsizei stride = this->layout.size;
    GLuint stream_id = 0; // 一个VAO是可以有多个顶点缓冲区的，目前先只用一个
    // 指定顶点缓冲区和索引
    glVertexArrayVertexBuffer(vao_id, stream_id, vertex_buffers->get_id(), 0, stride);
    glVertexArrayElementBuffer(vao_id, index_buffer->get_id());
    // 指定顶点格式
    for (const auto &[attribute, type, offset] : this->layout.attributes) {

        const auto [num_components, gl_type] = FieldType2OpenGLComponentsAndType(type);

        GLuint index = (GLuint)attribute;
        glEnableVertexArrayAttrib(vao_id, index);
        glVertexArrayAttribFormat(vao_id, index, num_components, gl_type, GL_FALSE, offset);
        glVertexArrayAttribBinding(vao_id, index, stream_id);
    }

    // 传递SubMesh信息
    this->submeshes.reserve(submeshes.size());
    for (size_t i = 0; i < submeshes.size(); i++) {
        this->submeshes.emplace_back(submeshes[i].start_index, submeshes[i].index_count,
                                     Topology2OpenGL(submeshes[i].topology));
    }

    opengl_debug_check_error();
}

} // namespace Graphics
} // namespace Goonya