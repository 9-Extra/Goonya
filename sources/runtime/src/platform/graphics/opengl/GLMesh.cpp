#include "GLMesh.h"

#include "platform/graphics/opengl/GLBuffer.h"

namespace Goonya {

static std::tuple<GLuint, GLenum> FieldType2OpenGLComponentsAndType(Meta::FieldType type) {
    switch (type) {

    case Meta::FieldType::nul:
        break;
    case Meta::FieldType::i32:
        return {1, GL_INT};
    case Meta::FieldType::i64:
        throw RuntimeError("Invalid Type for Vertex Array");
    case Meta::FieldType::u32:
        return {1, GL_UNSIGNED_INT};
    case Meta::FieldType::u64:
        throw RuntimeError("Invalid Type for Vertex Array");
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
    case Meta::FieldType::vec2i:
        return {2, GL_INT};
    case Meta::FieldType::vec3i:
        return {3, GL_INT};
    case Meta::FieldType::vec4i:
        return {4, GL_INT};
    case Meta::FieldType::mat4f:
        throw RuntimeError("Invalid Type for Vertex Array");
    }

    throw RuntimeError("Invalid Field Type");
}

static bool IsIntegerFieldType(Meta::FieldType type) {
    switch (type) {
    case Meta::FieldType::i32:
    case Meta::FieldType::u32:
    case Meta::FieldType::vec2i:
    case Meta::FieldType::vec3i:
    case Meta::FieldType::vec4i:
        return true;
    default:
        return false;
    }
}

GLMesh::GLMesh(VertexLayout layout) noexcept : layout(std::move(layout)) {
    glCreateVertexArrays(1, &vao_id); // 创建空的vao

    // 指定顶点格式
    for (const auto &[location, info] : this->layout.attributes) {
        const auto [type, stream_id, offset] = info;

        const auto [num_components, gl_type] = FieldType2OpenGLComponentsAndType(type);

        GLuint index = static_cast<GLuint>(location);
        glEnableVertexArrayAttrib(vao_id, index);
        if (IsIntegerFieldType(type)) {
            glVertexArrayAttribIFormat(vao_id, index, num_components, gl_type, offset);
        } else {
            glVertexArrayAttribFormat(vao_id, index, num_components, gl_type, GL_FALSE, offset);
        }
        glVertexArrayAttribBinding(vao_id, index, stream_id);
    }
};

void GLMesh::set_vertices(uint32_t stream_id, const std::span<const std::byte> &data) noexcept {
    GN_ASSERT(stream_id < layout.buffer_count());
    Ref<GLBuffer> buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, data);

    vertices_buffers[stream_id] = buffer;
    glVertexArrayVertexBuffer(vao_id, stream_id, buffer->get_id(), 0, layout.vertex_size[stream_id]);
}

void GLMesh::set_indices(const std::span<const uint32_t> &indices) noexcept {
    Ref<GLBuffer> buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::as_bytes(indices));
    indices_buffer = buffer;
    glVertexArrayElementBuffer(vao_id, buffer->get_id());
}

} // namespace Goonya
