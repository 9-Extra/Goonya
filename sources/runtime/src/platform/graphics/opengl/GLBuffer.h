#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

#include <glad/glad.h>

#include "core/RefCount.h"
#include "core/metatype/metatype.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

enum class BufferType { STATIC, DYNAMIC, STREAM, READBACK };
enum class BufferMapOption {
    WRITE_DISCARD, // 丢弃旧数据，避免复制
    WRITE_MODIFY,  // 修改旧数据，可能导致从显存到内存的复制
    READ_ONLY,     // 用于读
    READ_WRITE,    // 用于读写
};

static GLuint GLBufferType(BufferType type) {
    switch (type) {
    case BufferType::STATIC:
        return GL_STATIC_DRAW;
    case BufferType::DYNAMIC:
        return GL_DYNAMIC_DRAW;
    case BufferType::STREAM:
        return GL_STREAM_DRAW;
    case BufferType::READBACK:
        return GL_STATIC_READ;
    }

    throw RuntimeError("Invalid BufferType");
    return 0;
}

class GLBuffer final : public RefCount {
private:
    GLuint id = 0;

    size_t size;
    BufferType type;

public:
    GLBuffer(size_t size, BufferType type) : size(size), type(type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, nullptr, GLBufferType(type));
    };
    GLuint get_id() const noexcept { return id; }
    size_t get_size() const noexcept { return size; }
    BufferType get_type() const noexcept { return type; }

    // access
    // NOLINTNEXTLINE(readability-make-member-function-const)
    void write(std::span<const std::byte> data, size_t offset = 0) noexcept {
        assert(data.size_bytes() + offset <= size);
        glNamedBufferSubData(id, offset, data.size_bytes(), data.data());
    };
    std::byte *map(BufferMapOption option) const noexcept { return map_range(option, 0, size); };
    std::byte *map_range(BufferMapOption option, size_t offset, size_t size) const noexcept {
        if (size == 0) {
            return nullptr;
        }
        GLenum access = 0;
        switch (option) {
        case BufferMapOption::WRITE_DISCARD: {
            access = GL_MAP_WRITE_BIT;
            if (offset == 0 && size == this->size) {
                access |= GL_MAP_INVALIDATE_BUFFER_BIT; // 包含整个Buffer
            } else {
                access |= GL_MAP_INVALIDATE_RANGE_BIT; // 包含部分Buffer
            }
            break;
        }
        case BufferMapOption::WRITE_MODIFY: {
            access = GL_MAP_WRITE_BIT;
            break;
        }
        case BufferMapOption::READ_ONLY: {
            access = GL_MAP_READ_BIT;
            break;
        }
        case BufferMapOption::READ_WRITE: {
            access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            break;
        }
        }
        void *ptr = glMapNamedBufferRange(id, offset, size, access);
        assert(ptr);
        return reinterpret_cast<std::byte *>(ptr);
    };

    void unmap() const noexcept {
        if (size != 0) {
            glUnmapNamedBuffer(id);
        }
    };

    void invalidate() noexcept { glNamedBufferData(id, size, nullptr, GLBufferType(type)); }

    // bind
    void bind_uniform(uint32_t binding) const noexcept { glBindBufferBase(GL_UNIFORM_BUFFER, binding, id); }
    void bind_uniform_ranged(uint32_t binding, size_t offset, size_t size) const noexcept {
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, id, offset, size);
    }

    void bind_storage(uint32_t binding) const noexcept { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id); };
    void bind_storage_ranged(uint32_t binding, size_t offset, size_t size) const noexcept {
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, id, offset, size);
    };

    ~GLBuffer() { glDeleteBuffers(1, &id); }

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        glObjectLabel(GL_BUFFER, id, (GLsizei)name.size(), name.data());
#endif
    }
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBufferWriter {
private:
    GLBuffer *buffer;
    T *ptr;

public:
    StructBufferWriter(Ref<GLBuffer> buffer, BufferMapOption option)
        : buffer(buffer.get()), ptr(reinterpret_cast<T *>(buffer->map(option))) {
        assert(buffer);
    }
    StructBufferWriter(Ref<GLBuffer> buffer, BufferMapOption option, size_t offset)
        : buffer(buffer.get()), ptr(reinterpret_cast<T *>(buffer->map_range(option, offset, sizeof(T)))) {
        assert(buffer);
    }
    StructBufferWriter(StructBufferWriter &other) = delete;

    T *operator->() noexcept { return ptr; }

    ~StructBufferWriter() noexcept { buffer->unmap(); }
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
        assert(buffer);
        ptr = reinterpret_cast<T *>(buffer->map(option));
    }

    // 映射指定范围的缓冲区
    ArrayBufferWriter(Ref<GLBuffer> buffer, BufferMapOption option, size_t start_index, size_t element_count)
        : buffer(buffer.get()), element_count(element_count) {
        assert(buffer);
        size_t offset = start_index * sizeof(T);
        size_t size = element_count * sizeof(T);
        ptr = static_cast<T *>(buffer->map_range(option, offset, size));
    }

    // 禁止拷贝
    ArrayBufferWriter(const ArrayBufferWriter &) = delete;
    ArrayBufferWriter &operator=(const ArrayBufferWriter &) = delete;

    // 通过索引访问元素
    T *operator[](size_t index) noexcept {
        assert(index < element_count);
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
        assert(buffer);
    }
    DynamicBufferWriter(Ref<GLBuffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option, size_t offset)
        : Meta::DynamicStructWriter(layout, buffer->map_range(option, offset, layout.size)), buffer(buffer.get()) {
        assert(buffer);
    }
    DynamicBufferWriter(DynamicBufferWriter &other) = delete;

    ~DynamicBufferWriter() { buffer->unmap(); }
};

} // namespace Goonya
