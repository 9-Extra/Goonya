#pragma once

#include "core/hash_helper.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include <cstdint>
#include <vector>

namespace Goonya {
namespace Graphics {

/*
定义着色器编写规范：每一个Location绑定的数据都是指定的。可少不可多，名字可以改，类型不能变
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

如果着色器需要的数据Mesh中没有，UB。
*/

enum class VertexAttribute : uint32_t {
    POSITION = 0, // 指定location
    NORMAL = 1,
    TANGENT = 2,
    UV = 3,
};

struct VertexLayout {
    // 用途，类型，偏移量
    std::vector<std::tuple<VertexAttribute, Meta::FieldType, size_t>> attributes;
    size_t size; // 单个顶点大小

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

class Mesh : public intrusive_ptr_base<Mesh> {
public:
    virtual uint32_t get_submesh_count() const noexcept = 0;
    virtual ~Mesh() = default;
};

} // namespace Graphics
} // namespace Goonya