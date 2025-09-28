#pragma once

#include "core/RefCount.h"
#include "core/cgmath.h"
#include "core/metatype/metatype.h"
#include "resource/Resource.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <sys/types.h>
#include <utility>
#include <vector>

namespace Goonya::Graphics {

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

const Meta::FieldType VertexAttributeTypeMap[] = {
    Goonya::Meta::FieldType::vec3f, // position
    Goonya::Meta::FieldType::vec3f, // normal
    Goonya::Meta::FieldType::vec4f, // tangent
    Goonya::Meta::FieldType::vec3f, // color
    Goonya::Meta::FieldType::vec2f, // uv
};

struct VertexLayout {
    struct AttributeInfo {
        Meta::FieldType type;
        uint32_t stream_id; // 可以用多个底层Buffer装数据，此id指定装到哪一个buffer里
        uint32_t offset;
    };
    std::map<uint32_t, AttributeInfo> attributes; // location -> info
    std::vector<uint32_t> vertex_size;            // 每个buffer的单个顶点大小

    uint32_t buffer_count() const noexcept { return (uint32_t)vertex_size.size(); }

    bool operator==(const VertexLayout &b) const noexcept = default;
};

class VertexLayoutBuilder {
    VertexLayout layout{};
    uint32_t current_buffer_id = 0;
    bool used = false;

public:
    VertexLayoutBuilder() noexcept { select_buffer(0); };

    VertexLayoutBuilder &select_buffer(uint32_t stream_id) noexcept {
        assert(!used);
        if (stream_id >= layout.vertex_size.size()) {
            layout.vertex_size.resize(stream_id + 1);
            current_buffer_id = stream_id;
        }
        return *this;
    }

    VertexLayoutBuilder &add_attribute(uint32_t location, Meta::FieldType type) noexcept {
        assert(!used);
        assert(!layout.attributes.contains(location));

        uint32_t offset = layout.vertex_size[current_buffer_id];
        layout.vertex_size[current_buffer_id] += Meta::sizeof_field_type(type);
        layout.attributes[location] = {type, current_buffer_id, offset};

        return *this;
    }

    VertexLayoutBuilder &add_attribute(VertexAttribute attribute) noexcept {
        assert(attribute < VertexAttribute::MAX_ATTRIBUTE);
        add_attribute((uint32_t)attribute, VertexAttributeTypeMap[(uint32_t)attribute]);
        return *this;
    }

    VertexLayout build() noexcept {
        used = true;

        uint32_t remapped = 0;
        std::vector<uint32_t> stream_remap(layout.vertex_size.size()); // 消除大小为0的buffer
        std::vector<uint32_t> remapped_size;
        for (auto [i, size] : std::views::enumerate(layout.vertex_size)) {
            if (size != 0) {
                remapped_size.push_back(size);
                stream_remap[i] = remapped++;
            }
        }

        if (remapped == layout.vertex_size.size()) {
            return layout; // 不需要remap
        } else {
            // 进行重映射
            layout.vertex_size = std::move(remapped_size);
            for (auto &[_, info] : layout.attributes) {
                info.stream_id = stream_remap[info.stream_id];
            }
            return layout;
        }
    }
};

enum class Topology { POINT, LINE, TRIANGLE };

struct SubMesh {
    uint32_t start_index;
    uint32_t index_count;
    uint32_t base_vertex_offset = 0;
    Topology topology = Topology::TRIANGLE;

    BoundingBox aabb = BoundingBox::infinite();
};

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

class Mesh : public RefCount {
public:
    std::vector<SubMesh> submeshes;

protected:
    VertexLayout layout{};

public:
    explicit Mesh(VertexLayout layout) noexcept : layout(std::move(layout)) {}
    virtual ~Mesh() = default;

    virtual void bind() const noexcept = 0;

    virtual void set_vertices(uint32_t stream_id, const std::span<const std::byte> &data) noexcept = 0;
    virtual void set_indices(const std::span<const uint32_t> &indices) noexcept = 0;

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        _set_debug_label(name);
#endif
    }

protected:
    virtual void _set_debug_label(const std::string &name) const noexcept = 0;
};

class MeshContainer final : public Resource::ResourceContainer<Graphics::MeshDesc, Graphics::Mesh> {
public:
    MeshContainer() : ResourceContainer<MeshDesc, Mesh>("网格") {}

protected:
    Ref<Mesh> load(const MeshDesc &desc) const override;
};

} // namespace Goonya::Graphics
