#pragma once

#include <cassert>
#include <cstdint>
#include <span>

#include "../Buffer.h"
#include "GLBasic.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

static GLuint GLBufferType(BufferType type) {
    switch (type) {
    case STATIC:
        return GL_STATIC_DRAW;
    case DYNAMIC:
        return GL_DYNAMIC_DRAW;
    case STREAM:
        return GL_STREAM_DRAW;
    case READBACK:
        return GL_STATIC_READ;
    }

    throw RuntimeError("Invaild BufferType");
    return 0;
}

template <class T>
class GLBufferImpl : public T {
    // 为了防止反复写Buffer的基础实现所以写进了模板里，避免菱形继承（也可以使用组合的方式）
public:
    GLBufferImpl(uint32_t size, BufferType type) : size(size), type(type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, nullptr, GLBufferType(type));
        opengl_debug_check_error();
    };
    template <typename D>
    GLBufferImpl(std::span<const D> data, BufferType type) : size(data.size_bytes()), type(type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
        opengl_debug_check_error();
    };

    template <typename D>
    GLBufferImpl(std::span<D> data, BufferType type) : size(data.size_bytes()), type(type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
        opengl_debug_check_error();
    };

    GLuint get_id() const noexcept { return id; }

    virtual uint32_t get_size() const noexcept override { return size; }
    virtual BufferType get_type() const noexcept override { return type; }
    // access
    virtual void write(const std::span<uint8_t> data, uint32_t offset = 0) noexcept override {
        assert(data.size_bytes() + offset <= get_size());
        glNamedBufferSubData(id, offset, data.size_bytes(), data.data());
    };
    virtual void *map() const noexcept override {
        if (size != 0) {
            void *ptr = glMapNamedBuffer(id, GL_WRITE_ONLY);
            assert(ptr);
            return ptr;
        } else {
            return nullptr; // 对于大小为0的Buffer返回空指针
        }
    };
    virtual void unmap() const noexcept override { 
        if (size != 0){
            glUnmapNamedBuffer(id); 
        }
    };

    virtual ~GLBufferImpl() { glDeleteBuffers(1, &id); }

protected:
    GLuint id;

private:
    uint32_t size;
    BufferType type;
};

class GLBuffer : public GLBufferImpl<Buffer> {
public:
    using GLBufferImpl<Buffer>::GLBufferImpl;
};

class GLIndexBuffer : public GLBufferImpl<IndexBuffer> {
public:
    GLIndexBuffer(std::span<const uint16_t> indices)
        : GLBufferImpl<IndexBuffer>(indices, BufferType::STATIC), index_count(indices.size()) {}
    virtual uint32_t get_index_count() const noexcept { return index_count; };

private:
    uint32_t index_count;
};

class GLUniformBuffer : public GLBufferImpl<UniformBuffer> {
public:
    using GLBufferImpl<UniformBuffer>::GLBufferImpl;
    virtual void bind_uniform(uint32_t binding) const noexcept override {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }
};

class GLVertexBuffer : public GLBufferImpl<VertexBuffer> {
public:
    template <typename D>
    GLVertexBuffer(std::span<const D> data) : GLBufferImpl<VertexBuffer>(data, BufferType::STATIC) {}
};

} // namespace Graphics
} // namespace Goonya