#include "GLMesh.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include <cassert>
#include <cstdint>
#include <vector>

namespace Goonya {
namespace Graphics {

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

GLMesh::GLMesh(Topology topology, VertexLayout layout, intrusive_ptr<Buffer> vertex_buffers,
               intrusive_ptr<Buffer> index_buffer)
    : GLMesh(std::vector<SubMesh>{SubMesh{0, (uint32_t)index_buffer->get_size(), topology}}, layout, vertex_buffers,
           index_buffer) {}

GLMesh::GLMesh(const std::vector<SubMesh> &submeshes, VertexLayout layout, intrusive_ptr<Buffer> vertex_buffers,
               intrusive_ptr<Buffer> index_buffer)
    : Mesh{submeshes, layout, vertex_buffers, index_buffer} {
    
    intrusive_ptr<GLBuffer> gl_vertex_buffers = dynamic_intrusive_ptr_cast<GLBuffer>(vertex_buffers);
    intrusive_ptr<GLBuffer> gl_indices_buffers = dynamic_intrusive_ptr_cast<GLBuffer>(index_buffer);
    assert(gl_vertex_buffers && gl_indices_buffers);
    
    // 使用顶点格式创建VAO
    glCreateVertexArrays(1, &vao_id);
    GLsizei stride = this->layout.size;
    GLuint stream_id = 0; // 一个VAO是可以有多个顶点缓冲区的，目前先只用一个

    // 指定顶点缓冲区和索引
    glVertexArrayVertexBuffer(vao_id, stream_id, gl_vertex_buffers->get_id(), 0, stride);
    glVertexArrayElementBuffer(vao_id, gl_indices_buffers->get_id());
    // 指定顶点格式
    for (const auto &[attribute, type, offset] : this->layout.attributes) {

        const auto [num_components, gl_type] = FieldType2OpenGLComponentsAndType(type);

        GLuint index = (GLuint)attribute;
        glEnableVertexArrayAttrib(vao_id, index);
        glVertexArrayAttribFormat(vao_id, index, num_components, gl_type, GL_FALSE, offset);
        glVertexArrayAttribBinding(vao_id, index, stream_id);
    }
}

} // namespace Graphics
} // namespace Goonya