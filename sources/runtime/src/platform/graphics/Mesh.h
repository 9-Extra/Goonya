#pragma once

#include "core/Bytes.h"
#include "core/hash_helper.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace Goonya::Graphics {

/*
定义着色器编写规范：每一个Location绑定的数据都是指定的。可少不可多，名字可以改，类型不能变
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
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
};

struct VertexLayout {
    // 用途，类型，偏移量
    std::vector<std::tuple<VertexAttribute, Meta::FieldType, uint32_t>> attributes;
    uint32_t size = 0; // 单个顶点大小

    bool operator==(const VertexLayout &b) const noexcept = default;
    size_t hash() const noexcept {
        size_t seed = size;
        for (const auto &t : attributes) {
            hash_combine(seed, hash_tuple(t));
        }
        return seed;
    }
};

enum class Topology { POINT, LINE, TRIANGLE };

struct SubMesh {
    uint32_t start_index;
    uint32_t index_count;
    Topology topology;
};

struct MeshDesc {
    VertexLayout vertex_layout;
    Bytes raw_vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> sub_meshes;

    template <typename T, typename I, typename M>
    MeshDesc(T &&vertex_layout, Bytes &&raw_vertices, I &&indices, M &&sub_meshes)
        : vertex_layout(std::forward<T>(vertex_layout)), raw_vertices(std::move(raw_vertices)),
          indices(std::forward<I>(indices)), sub_meshes(std::forward<M>(sub_meshes)) {}
    template <typename T, typename I>
    MeshDesc(T &&vertex_layout, Bytes &&raw_vertices, I &&indices, Topology topology)
        : vertex_layout(std::forward<T>(vertex_layout)), raw_vertices(std::move(raw_vertices)),
          indices(std::forward<I>(indices)), sub_meshes({{0, static_cast<uint32_t>(this->indices.size()), topology}}) {}
};

class Mesh : public intrusive_ptr_base<Mesh> {
    /* 类内容的大致声明顺序是：公开内部类，公开字段，私有字段，公开方法，私有方法 */
public:
    std::vector<SubMesh> submeshes;
protected:
    VertexLayout layout;
    intrusive_ptr<Buffer> vertex_buffer;
    intrusive_ptr<Buffer> indices_buffer;

    mutable bool is_dirty = true;
public:
    
    virtual ~Mesh() = default;

    virtual void bind() const noexcept = 0;

    const VertexLayout &get_layout() const noexcept { return layout; }

    template <typename T>
        requires std::is_convertible_v<T, VertexLayout>
    void set_layout(T &&layout) noexcept {
        this->layout = std::forward<T>(layout);
        is_dirty = true;
    }
    void set_vertex_buffer(const intrusive_ptr<Buffer> &vertex_buffer) noexcept {
        assert(vertex_buffer);
        this->vertex_buffer = vertex_buffer;
        is_dirty = true;
    }
    void set_indices_buffer(const intrusive_ptr<Buffer> &indices_buffer) noexcept {
        assert(indices_buffer);
        this->indices_buffer = indices_buffer;
        is_dirty = true;
    }

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        _set_debug_label(name);
#endif
    }

protected:
    virtual void _set_debug_label(const std::string &name) const noexcept = 0;
};

} // namespace Goonya::Graphics
