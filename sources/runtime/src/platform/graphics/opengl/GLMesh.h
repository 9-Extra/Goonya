#pragma once

#include "GLBuffer.h"
#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/cgmath/vector.h"
#include "core/metatype/metatype.h"
#include "resource/Resource.h"

#include <array>

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <sys/types.h>
#include <utility>
#include <vector>

namespace Goonya {

/*
定义着色器编写规范：每一个Location绑定的数据都是指定的。可少不可多，名字可以改，类型不能变
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

如果着色器需要的数据Mesh中没有，UB。
*/

/*
UV应以左上角为(0,0)点，即
(0,0)-(1,0)
  |     |
(0,1)-(1,1)
这与DX的内部设定相同，与OpenGL的上下相反
*/

enum class VertexAttribute : uint32_t {
    POSITION = 0, // 指定location
    NORMAL = 1,
    TANGENT = 2,
    COLOR = 3,
    UV = 4,

    MAX_ATTRIBUTE
};

struct VertexLayout {
    struct AttributeInfo {
        Meta::FieldType type = Meta::FieldType::nul; // 为nul表示此location没有绑定属性
        uint32_t stream_id;                          // 可以用多个底层Buffer装数据，此id指定装到哪一个buffer里
        uint32_t offset;
    };
    constexpr static size_t MAX_VERTEX_ATTRIBUTES = 16; // 参考GL_MAX_VERTEX_ATTRIBS多数硬件就是16

    std::array<AttributeInfo, MAX_VERTEX_ATTRIBUTES> attributes; // location -> info
    std::array<uint32_t, MAX_VERTEX_ATTRIBUTES>
        vertex_stride; // stream_id -> buffer内单个顶点大小，为0表示此stream无buffer绑定

    uint32_t buffer_count() const noexcept {
        uint32_t count = 0;
        for (uint32_t stride : vertex_stride) {
            if (stride != 0) {
                count++;
            }
        }
        return count;
    }

    bool operator==(const VertexLayout &b) const noexcept = default;
};

// 点和线应该弃用，只支持三角形
enum class Topology { TRIANGLE, TRIANGLE_STRIP, TRIANGLE_FAN };

struct SubMesh {
    uint32_t start_index;
    uint32_t index_count;
    uint32_t base_vertex_offset = 0;
    Topology topology = Topology::TRIANGLE;

    BoundingBox aabb = BoundingBox::infinite();
};

/*
struct MeshDesc {
    VertexLayout vertex_layout;
    std::vector<std::byte> raw_vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> sub_meshes;

    template <typename T, typename I, typename M>
    MeshDesc(T &&vertex_layout, std::vector<std::byte> &&raw_vertices, I &&indices, M &&sub_meshes)
        : vertex_layout(std::forward<T>(vertex_layout)), raw_vertices(std::move(raw_vertices)),
          indices(std::forward<I>(indices)), sub_meshes(std::forward<M>(sub_meshes)) {}
    template <typename T, typename I>
    MeshDesc(T &&vertex_layout, std::vector<std::byte> &&raw_vertices, I &&indices, Topology topology, BoundingBox aabb)
        : vertex_layout(std::forward<T>(vertex_layout)), raw_vertices(std::move(raw_vertices)),
          indices(std::forward<I>(indices)),
          sub_meshes({{0, static_cast<uint32_t>(this->indices.size()), 0, topology, aabb}}) {}
};
*/

/**
 * @brief VAO的封装，可以移动
 * @note
 * 作为轻量对象，内部不增加Ref<GLBuffer>的引用计数（OpenGL内部可能会，但是也不一定），需要外部保证Ref<GLBuffer>存活
 */
class GLVertexLayout {
private:
    GLuint id = 0;

public:
    GLVertexLayout() = default;
    explicit GLVertexLayout(const VertexLayout &layout);
    GLVertexLayout(GLVertexLayout &) = delete;
    GLVertexLayout(GLVertexLayout &&other) noexcept : id(std::exchange(other.id, 0)) {}
    ~GLVertexLayout() { glDeleteVertexArrays(1, &id); }

    GLVertexLayout &operator=(GLVertexLayout &&other) noexcept {
        if (other.id == id) return *this;
        glDeleteVertexArrays(1, &id);
        id = std::exchange(other.id, 0);
        return *this;
    }

    void bind() const noexcept { glBindVertexArray(id); }
    // NOLINTNEXTLINE(readability-make-member-function-const)
    void set_vertice_buffer(uint32_t stream_idx, const Ref<GLBuffer> &buffer, int32_t offset, int32_t stride) noexcept {
        glVertexArrayVertexBuffer(id, stream_idx, buffer->get_id(), offset, stride);
    }
    // NOLINTNEXTLINE(readability-make-member-function-const)
    void set_index_buffer(const Ref<GLBuffer> &buffer) noexcept { glVertexArrayElementBuffer(id, buffer->get_id()); }

    // 判断是否为空
    explicit operator bool() const noexcept { return id != 0; }
};

class GLMesh final : public Resource {
private:
    Ref<GLBuffer> mesh_buffer; // position, normal, tangent
    Ref<GLBuffer> skin_buffer; // uv, color, joints, weight
    Ref<GLBuffer> indices_buffer;

    // 顶点属性可以少不可以多，顺序严格排序，真实的布局在构建网格体时由其中的属性组合唯一确定
    VertexLayout layout;
    size_t vertex_count = 0;

    GLVertexLayout vao;

public:
    std::vector<SubMesh> submeshes;

    GLMesh(const VertexLayout &layout, size_t vertex_count, Ref<GLBuffer> mesh_buffer, Ref<GLBuffer> skin_buffer,
           Ref<GLBuffer> indices_buffer)
        : mesh_buffer(std::move(mesh_buffer)), skin_buffer(std::move(skin_buffer)),
          indices_buffer(std::move(indices_buffer)), layout(layout), vertex_count(vertex_count) {
        init_vao();
    }

    void bind() const noexcept { vao.bind(); }

    size_t get_vertex_count() const noexcept { return vertex_count; }
    size_t get_index_count() const noexcept { return indices_buffer ? indices_buffer->get_size() : 0; }

private:
    void init_vao() {
        vao = GLVertexLayout{layout};
        if (mesh_buffer) {
            vao.set_vertice_buffer(0, mesh_buffer, 0, layout.vertex_stride[0]);
        }
        if (skin_buffer) {
            vao.set_vertice_buffer(1, skin_buffer, 0, layout.vertex_stride[1]);
        }
        if (indices_buffer) {
            vao.set_index_buffer(indices_buffer);
        }
    }
};

class MeshBuilder final {
public:
    std::vector<Vector3f> position;
    std::vector<Vector3f> normal;
    std::vector<Vector4f> tangent;

    std::vector<Vector2f> uv;
    std::vector<Vector3f> color;
    std::vector<Vector4i> joints;
    std::vector<Vector4f> weight;

    std::vector<uint32_t> indices;

    std::vector<SubMesh> submeshes;

private:
    constexpr static Meta::FieldType VertexAttributeTypeMap[] = {
        Goonya::Meta::FieldType::vec3f, // position
        Goonya::Meta::FieldType::vec3f, // normal
        Goonya::Meta::FieldType::vec4f, // tangent
        Goonya::Meta::FieldType::vec3f, // color
        Goonya::Meta::FieldType::vec2f, // uv
    };

    static void add_attribute(VertexLayout &layout, VertexAttribute attribute, uint32_t stream_idx) noexcept {
        uint32_t location = (uint32_t)attribute;
        Meta::FieldType type = VertexAttributeTypeMap[location];
        uint32_t offset = layout.vertex_stride[stream_idx];
        layout.vertex_stride[stream_idx] += (uint32_t)Meta::sizeof_field_type(type);
        layout.attributes[location] = {type, stream_idx, offset};
    }

public:
    MeshBuilder() noexcept = default;

    /**
     * @brief 生成空的GLMesh
     */
    static Ref<GLMesh> build_empty();

    Ref<GLMesh> build(bool verify = false) const;
};

} // namespace Goonya
