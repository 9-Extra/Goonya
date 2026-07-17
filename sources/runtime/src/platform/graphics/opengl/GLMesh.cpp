#include "GLMesh.h"

#include "core/metatype/metatype.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include <algorithm>
#include <cstdint>
#include <span>

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

Ref<GLMesh> MeshBuilder::build_empty() {
    VertexLayout layout{};
    Ref<GLBuffer> mesh_buffer;
    Ref<GLBuffer> skin_buffer;
    Ref<GLBuffer> indices_buffer;

    return create_ref<GLMesh>(layout, 0, std::move(mesh_buffer), std::move(skin_buffer), std::move(indices_buffer));
}

Ref<GLMesh> MeshBuilder::build(bool verify) const {
    GN_ASSERT(!position.empty());
    size_t vertex_count = position.size();

    // 规模一致性校验
    GN_ASSERT(normal.empty() || normal.size() == vertex_count);
    GN_ASSERT(tangent.empty() || tangent.size() == vertex_count);
    GN_ASSERT(uv.empty() || uv.size() == vertex_count);
    GN_ASSERT(color.empty() || color.size() == vertex_count);
    if (verify) {
        GN_ASSERT(std::ranges::all_of(indices, [vertex_count](uint32_t i) { return i < vertex_count; }));
    }
    // 构建VertexLayout
    VertexLayout layout{};

    if (!position.empty()) add_attribute(layout, VertexAttribute::POSITION, 0);
    if (!normal.empty()) add_attribute(layout, VertexAttribute::NORMAL, 0);
    if (!tangent.empty()) add_attribute(layout, VertexAttribute::TANGENT, 0);
    if (!color.empty()) add_attribute(layout, VertexAttribute::COLOR, 1);
    if (!uv.empty()) add_attribute(layout, VertexAttribute::UV, 1);

    // 构建mesh_buffer (stream 0: position + normal + tangent)
    std::vector<std::byte> mesh_data;
    uint32_t mesh_stride = layout.vertex_stride[0];
    if (mesh_stride > 0) {
        mesh_data.resize(mesh_stride * vertex_count);
        for (size_t i = 0; i < vertex_count; ++i) {
            size_t base = i * mesh_stride;
            if (!position.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::POSITION].offset;
                std::memcpy(mesh_data.data() + base + offset, &position[i], sizeof(Vector3f));
            }
            if (!normal.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::NORMAL].offset;
                std::memcpy(mesh_data.data() + base + offset, &normal[i], sizeof(Vector3f));
            }
            if (!tangent.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::TANGENT].offset;
                std::memcpy(mesh_data.data() + base + offset, &tangent[i], sizeof(Vector4f));
            }
        }
    }

    // 构建skin_buffer (stream 1: color + uv)
    std::vector<std::byte> skin_data;
    uint32_t skin_stride = layout.vertex_stride[1];
    if (skin_stride > 0) {
        skin_data.resize(skin_stride * vertex_count);
        for (size_t i = 0; i < vertex_count; ++i) {
            size_t base = i * skin_stride;
            if (!color.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::COLOR].offset;
                std::memcpy(skin_data.data() + base + offset, &color[i], sizeof(Vector3f));
            }
            if (!uv.empty()) {
                auto offset = layout.attributes[(size_t)VertexAttribute::UV].offset;
                std::memcpy(skin_data.data() + base + offset, &uv[i], sizeof(Vector2f));
            }
        }
    }

    // 创建GPU Buffer
    Ref<GLBuffer> mesh_buffer;
    if (!mesh_data.empty()) {
        mesh_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::span(mesh_data));
    }

    Ref<GLBuffer> skin_buffer;
    if (!skin_data.empty()) {
        skin_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, std::span(skin_data));
    }

    std::span index_bytes{std::as_bytes(std::span(indices))};
    auto indices_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, index_bytes);

    // 构建GLMesh
    auto mesh = create_ref<GLMesh>(layout, vertex_count, std::move(mesh_buffer), std::move(skin_buffer),
                                   std::move(indices_buffer));

    mesh->submeshes = submeshes;
    return mesh;
}

} // namespace Goonya
