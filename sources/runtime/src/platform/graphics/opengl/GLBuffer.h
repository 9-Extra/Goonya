#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

#include "../Buffer.h"
#include "GLBasic.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

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

    throw RuntimeError("Invaild BufferType");
    return 0;
}

class GLBuffer : public Buffer {
    // 为了防止反复写Buffer的基础实现所以写进了模板里，避免菱形继承（也可以使用组合的方式）
public:
    GLBuffer(size_t size, BufferType type) : Buffer(size, type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, nullptr, GLBufferType(type));
        opengl_debug_check_error();
    };
    template <typename D>
    GLBuffer(std::span<const D> data, BufferType type) : Buffer(data.size_bytes(), type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
        opengl_debug_check_error();
    };

    template <typename D>
    GLBuffer(std::span<D> data, BufferType type) : Buffer(data.size_bytes(), type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
        opengl_debug_check_error();
    };

    GLuint get_id() const noexcept { return id; }

    // access
    virtual void write(const std::span<uint8_t> data, size_t offset = 0) noexcept override {
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

    // bind
    virtual void bind_uniform(uint32_t binding) const noexcept override{
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }

    virtual ~GLBuffer() { glDeleteBuffers(1, &id); }

protected:
    GLuint id;
};

} // namespace Graphics
} // namespace Goonya