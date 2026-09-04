#pragma once

#include "GLBuffer.h"
#include "core/RefCount.h"
#include "core/cgmath/aabb.h"
#include "core/metatype/metatype.h"

#include <array>

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <sys/types.h>
#include <utility>

namespace Goonya {

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

    bool has_attribute(size_t location) const noexcept { return attributes[location].type != Meta::FieldType::nul; }

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

} // namespace Goonya
