#include "GLBuffer.h"

namespace Goonya {
namespace Graphics {

static std::tuple<GLuint, GLenum> FieldType2OpenGLComponentsAndType(Meta::FieldType type){
    switch (type) {

    case Meta::FieldType::nul: break; 
    case Meta::FieldType::i32: return {1, GL_INT};
    case Meta::FieldType::i64: throw RuntimeError("Invaild Type for Vertex Array");
    case Meta::FieldType::u32: return {1,  GL_UNSIGNED_INT};
    case Meta::FieldType::u64: throw RuntimeError("Invaild Type for Vertex Array");
    case Meta::FieldType::f32: return {1, GL_FLOAT};
    case Meta::FieldType::f64: return {1, GL_DOUBLE};
    case Meta::FieldType::vec2f: return {2, GL_FLOAT};
    case Meta::FieldType::vec3f: return {3, GL_FLOAT};
    case Meta::FieldType::vec4f: return {4, GL_FLOAT};
    case Meta::FieldType::mat4f: throw RuntimeError("Invaild Type for Vertex Array");
    }

    throw RuntimeError("Invaild Field Type"); 
}

void GLVertexBuffer::bind_vertices() const {
    GLsizei stride = vertex_layout.size;
    glBindVertexBuffer(0, id, 0, stride);
    for (const auto &[binding, name ,type, offset] : vertex_layout.attributes) {
        
        const auto [num_components, gl_type] = FieldType2OpenGLComponentsAndType(type);
        
        glEnableVertexAttribArray(binding);
        glVertexAttribFormat(binding, num_components, gl_type, GL_FALSE, offset);
        glVertexAttribBinding(binding, 0);
    }
}
} // namespace Graphics
}