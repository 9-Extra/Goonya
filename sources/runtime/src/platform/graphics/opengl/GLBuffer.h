#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <glad/glad.h>
#include <type_traits>
#include <vector>

#include "core/RefCount.h"
#include "core/metatype/metatype.h"
#include "runtime/GAssert.h"

namespace Goonya {

enum class BufferType { DEVICE_ONLY, MODIFIABLE, READBACK, MEMORY };
enum class BufferMapOption {
    WRITE_DISCARD, // 丢弃旧数据，避免复制
    WRITE_MODIFY,  // 修改旧数据，可能导致从显存到内存的复制
    READ_ONLY,     // 用于读
};

static GLuint GLBufferType(BufferType type) {
    switch (type) {
    case BufferType::DEVICE_ONLY:
        return 0;
    case BufferType::MODIFIABLE:
        return GL_MAP_WRITE_BIT;
    case BufferType::READBACK:
        return GL_MAP_READ_BIT;
    case BufferType::MEMORY:
        return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT;
    default:
        std::unreachable();
    }
}

class GLBuffer final : public RefCount {
private:
    GLuint id = 0;
    size_t size;
    BufferType type;

public:
    GLBuffer(BufferType type, size_t size) : size(size), type(type) {
        if (size != 0) {
            glCreateBuffers(1, &id);
            glNamedBufferStorage(id, size, nullptr, GLBufferType(type));
        }
    };
    GLBuffer(BufferType type, std::span<const std::byte> data) : size(data.size_bytes()), type(type) {
        if (size != 0) {
            glCreateBuffers(1, &id);
            glNamedBufferStorage(id, size, data.data(), GLBufferType(type));
        }
    };

    GLuint get_id() const noexcept { return id; }
    size_t get_size() const noexcept { return size; }
    // access
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void write(const T *data, BufferMapOption option, size_t offset = 0) noexcept {
        write(std::span<const std::byte>((const std::byte *)data, sizeof(T)), option, offset);
    }
    // NOLINTNEXTLINE(readability-make-member-function-const)
    void write(std::span<const std::byte> data, BufferMapOption option, size_t offset = 0) noexcept;
    std::byte *map(BufferMapOption option) const noexcept { return map_range(option, 0, size); };
    std::byte *map_range(BufferMapOption option, size_t offset, size_t size) const noexcept;

    // NOLINTNEXTLINE(readability-make-member-function-const)
    void copy_from(Ref<GLBuffer> &src, size_t size, size_t src_offset = 0, size_t dst_offset = 0) noexcept {
        GN_ASSERT(src_offset + size <= src->size && dst_offset + size <= this->size);
        glCopyNamedBufferSubData(src->id, id, src_offset, dst_offset, size);
    }

    std::vector<std::byte> read(size_t size, size_t offset = 0) const noexcept {
        size = std::min(size, this->size - offset);
        std::vector<std::byte> data(size);
        if (size != 0) {
            glGetNamedBufferSubData(id, offset, size, data.data()); // 无论初始是否设置为可读，总是可以读取
        }
        return data;
    }
    void unmap() const noexcept {
        if (size != 0) {
            glUnmapNamedBuffer(id);
        }
    };

    // NOLINTNEXTLINE(readability-make-member-function-const)
    void invalidate() noexcept {
        if (size != 0) glInvalidateBufferData(id);
    }

    // bind
    void bind_uniform(uint32_t binding) const noexcept { glBindBufferBase(GL_UNIFORM_BUFFER, binding, id); }
    void bind_uniform_ranged(uint32_t binding, size_t offset, size_t size) const noexcept {
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, id, offset, size);
    }

    void bind_storage(uint32_t binding) const noexcept { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id); };
    void bind_storage_ranged(uint32_t binding, size_t offset, size_t size) const noexcept {
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, id, offset, size);
    };
    void bind_vertice_buffer(uint32_t stream_idx, int32_t offset, int32_t stride) const noexcept {
        glBindVertexBuffer(stream_idx, id, offset, stride);
    }
    void bind_index_buffer() const noexcept { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id); }

    ~GLBuffer() {
        if (size != 0) glDeleteBuffers(1, &id);
    }

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        if (size != 0) {
            glObjectLabel(GL_BUFFER, id, (GLsizei)name.size(), name.data());
        }
#endif
    }
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBytesAccessor {
private:
    T *ptr;

public:
    explicit StructBytesAccessor(void *ptr) : ptr(reinterpret_cast<T *>(ptr)) { GN_ASSERT(ptr); }

    StructBytesAccessor(StructBytesAccessor &other) = delete;

    T *operator->() noexcept { return ptr; }
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBufferAccessor : public StructBytesAccessor<T> {
private:
    GLBuffer *buffer;

public:
    StructBufferAccessor(Ref<GLBuffer> buffer, BufferMapOption option)
        : StructBytesAccessor<T>(reinterpret_cast<T *>(buffer->map(option))), buffer(buffer.get()) {
        GN_ASSERT(buffer);
    }
    StructBufferAccessor(Ref<GLBuffer> buffer, BufferMapOption option, size_t offset)
        : StructBytesAccessor<T>(reinterpret_cast<T *>(buffer->map_range(option, offset, sizeof(T)))),
          buffer(buffer.get()) {
        GN_ASSERT(buffer);
    }

    ~StructBufferAccessor() noexcept { buffer->unmap(); }
};

// 同上，但支持一个元素类型为T的动态大小的数组
template <class T>
class ArrayBufferWriter {
private:
    GLBuffer *buffer;
    T *ptr;
    size_t element_count;

public:
    // 映射整个缓冲区
    ArrayBufferWriter(Ref<GLBuffer> buffer, BufferMapOption option)
        : buffer(buffer.get()), element_count(buffer->get_size() / sizeof(T)) {
        GN_ASSERT(buffer);
        ptr = reinterpret_cast<T *>(buffer->map(option));
    }

    // 映射指定范围的缓冲区
    ArrayBufferWriter(Ref<GLBuffer> buffer, BufferMapOption option, size_t start_index, size_t element_count)
        : buffer(buffer.get()), element_count(element_count) {
        GN_ASSERT(buffer);
        size_t offset = start_index * sizeof(T);
        size_t size = element_count * sizeof(T);
        ptr = static_cast<T *>(buffer->map_range(option, offset, size));
    }

    // 禁止拷贝
    ArrayBufferWriter(const ArrayBufferWriter &) = delete;
    ArrayBufferWriter &operator=(const ArrayBufferWriter &) = delete;

    // 通过索引访问元素
    T *operator[](size_t index) noexcept {
        GN_ASSERT(index < element_count);
        return ptr + index;
    }

    // 获取元素数量
    size_t size() const noexcept { return element_count; }

    // 获取原始指针
    T *data() noexcept { return ptr; }

    ~ArrayBufferWriter() noexcept { buffer->unmap(); }
};

class DynamicBufferWriter : public Meta::DynamicStructWriter {
private:
    GLBuffer *buffer;

public:
    DynamicBufferWriter(Ref<GLBuffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option)
        : Meta::DynamicStructWriter(layout, buffer->map(option)), buffer(buffer.get()) {
        GN_ASSERT(buffer);
    }
    DynamicBufferWriter(Ref<GLBuffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option, size_t offset)
        : Meta::DynamicStructWriter(layout, buffer->map_range(option, offset, layout.size)), buffer(buffer.get()) {
        GN_ASSERT(buffer);
    }
    DynamicBufferWriter(DynamicBufferWriter &other) = delete;

    ~DynamicBufferWriter() { buffer->unmap(); }
};

} // namespace Goonya
