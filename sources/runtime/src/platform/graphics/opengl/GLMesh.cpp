#include "GLMesh.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include <cassert>

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

void GLMesh::update_VAO() const noexcept {
    intrusive_ptr<GLBuffer> gl_vertex_buffer = dynamic_intrusive_ptr_cast<GLBuffer>(vertex_buffer);
    intrusive_ptr<GLBuffer> gl_indices_buffer = dynamic_intrusive_ptr_cast<GLBuffer>(indices_buffer);
    assert(gl_vertex_buffer && gl_indices_buffer);
    assert(layout.size != 0); // Layout记得设置

    GLsizei stride = this->layout.size;
    GLuint stream_id = 0; // 一个VAO是可以有多个顶点缓冲区的，目前先只用一个

    // 指定顶点缓冲区和索引
    glVertexArrayVertexBuffer(vao_id, stream_id, gl_vertex_buffer->get_id(), 0, stride);
    glVertexArrayElementBuffer(vao_id, gl_indices_buffer->get_id());
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