#include "GLMesh.h"

#include "core/metatype/metatype.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include <ranges>

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

GLVertexLayout::GLVertexLayout(const VertexLayout &layout) {
    glCreateVertexArrays(1, &id); // 创建空的vao

    // 指定顶点格式
    for (const auto &[location, info] : std::views::enumerate(layout.attributes)) {
        const auto [type, stream_id, offset] = info;
        if (type == Meta::FieldType::nul) {
            continue;
        }

        const auto [num_components, gl_type] = FieldType2OpenGLComponentsAndType(type);

        glEnableVertexArrayAttrib(id, location);
        if (IsIntegerFieldType(type)) {
            glVertexArrayAttribIFormat(id, location, num_components, gl_type, offset);
        } else {
            glVertexArrayAttribFormat(id, location, num_components, gl_type, GL_FALSE, offset);
        }
        glVertexArrayAttribBinding(id, location, stream_id);
    }
};

} // namespace Goonya
