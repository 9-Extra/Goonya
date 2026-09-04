#pragma once

#include "core/RefCount.h"
#include "core/cgmath/vector.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/Resource.h"
#include "runtime/GAssert.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

namespace Goonya {

/*
定义着色器编写规范：每一个Location绑定的数据都是指定的。可少不可多，名字可以改，类型不能变
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

如果着色器需要的数据Mesh中没有，UB。
使用者唯一能自由指定的是某一种attribute是否存在，不能定义类型，位置和流
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

// 每个顶点属性的类型
constexpr static Meta::FieldType VertexAttributeTypeMap[] = {
    Goonya::Meta::FieldType::vec3f, // position
    Goonya::Meta::FieldType::vec3f, // normal
    Goonya::Meta::FieldType::vec4f, // tangent
    Goonya::Meta::FieldType::vec3f, // color
    Goonya::Meta::FieldType::vec2f, // uv
};

// 每个顶点属性保存的stream_id： 0: mesh_buffer，1: color_buffer
constexpr static size_t VertexAttributeStream[] = {
    0, // position
    0, // normal
    0, // tangent
    1, // color
    1, // uv
};

struct SkinVertex {
    Vector4i joints;
    Vector4f weights;
};

// 蒙皮网格的结构规定死，caculated_mesh_buffer必须长这样
struct SkinMeshVertex {
    Vector3f position;
    Vector3f normal;
    Vector4f tangent;
};

struct MeshDataArrays {
    std::vector<Vector3f> position;
    std::vector<Vector3f> normal;
    std::vector<Vector4f> tangent;

    std::vector<Vector2f> uv;
    std::vector<Vector3f> color;

    std::vector<uint32_t> indices;
    std::optional<std::vector<SubMesh>> submeshes;
};

class Mesh : public Resource {
protected:
    // 基本属性
    VertexLayout layout;
    size_t vertex_count = 0;
    std::vector<SubMesh> submeshes;

    // ----------GPU侧数据-----------
    Ref<GLBuffer> mesh_buffer;  // position, normal, tangent
    Ref<GLBuffer> color_buffer; // uv, color
    Ref<GLBuffer> indices_buffer;
    GLVertexLayout vao;

public:
    // 初始化为空
    Mesh() noexcept = default;
    explicit Mesh(const MeshDataArrays &data) : Mesh{} {
        Mesh::reconstruct(data); // 这是有个"虚"函数，子类慎用本方法构造基类
    }

    void bind() const noexcept {
        GN_ASSERT_MSG(vao, "在绑定Mesh前必须初始化");
        vao.bind();
    }

    size_t get_vertex_count() const noexcept { return vertex_count; }
    size_t get_index_count() const noexcept {
        return indices_buffer ? indices_buffer->get_size() / sizeof(uint32_t) : 0;
    }

    const std::vector<SubMesh> &get_submeshes() const noexcept { return submeshes; }
    bool has_attribute(VertexAttribute attribute) const noexcept {
        return layout.has_attribute(std::to_underlying(attribute));
    }

    void set_indices_data(std::span<const uint32_t> src) {
        std::span index_bytes{std::as_bytes(src)};
        indices_buffer = create_ref<GLBuffer>(BufferType::DEVICE_ONLY, index_bytes);
        vao.set_index_buffer(indices_buffer);
    }
    /**
     * @brief 上传数据
     */
    void reconstruct(const MeshDataArrays &data);

    /**
     * @brief 初始化一个空但是可绑定绘制的网格体
     */
    void set_empty() {
        layout = {};
        vao = GLVertexLayout{layout};
        vertex_count = 0;
        submeshes = {};

        mesh_buffer.reset();  // position, normal, tangent
        color_buffer.reset(); // uv, color
        indices_buffer.reset();
    }

protected:
    // 用于子类创建，所有GLBuffer拷贝引用，VAO独立
    Mesh(const Mesh &mesh)
        : layout(mesh.layout), vertex_count(mesh.vertex_count), submeshes(mesh.submeshes),
          mesh_buffer(mesh.mesh_buffer), color_buffer(mesh.color_buffer), indices_buffer(mesh.indices_buffer) {
        // VAO需要由子类创建
    }

    // 网格重建钩子
    virtual void on_reconstruct() {}

private:
    static void add_attribute(VertexLayout &layout, VertexAttribute attribute) noexcept {
        uint32_t location = (uint32_t)attribute;
        Meta::FieldType type = VertexAttributeTypeMap[location];
        uint32_t stream_idx = VertexAttributeStream[location];
        uint32_t offset = layout.vertex_stride[stream_idx];
        layout.vertex_stride[stream_idx] += (uint32_t)Meta::sizeof_field_type(type);
        layout.attributes[location] = {type, stream_idx, offset};
    }
};

} // namespace Goonya