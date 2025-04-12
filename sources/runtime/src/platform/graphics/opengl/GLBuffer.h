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
public:
    GLBuffer(size_t size, BufferType type) : Buffer(size, type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, nullptr, GLBufferType(type));
    };
    template <typename D>
    GLBuffer(std::span<const D> data, BufferType type) : Buffer(data.size_bytes(), type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
    };

    template <typename D>
    GLBuffer(std::span<D> data, BufferType type) : Buffer(data.size_bytes(), type) {
        glCreateBuffers(1, &id);
        glNamedBufferData(id, size, (void *)data.data(), GLBufferType(type));
    };

    GLuint get_id() const noexcept { return id; }

    // access
    virtual void write(std::span<const uint8_t> data, size_t offset = 0) noexcept override {
        assert(data.size_bytes() + offset <= get_size());
        glNamedBufferSubData(id, offset, data.size_bytes(), data.data());
    };
    virtual void *map(BufferMapOption option) const noexcept override { return map_range(option, 0, this->size); };
    virtual void *map_range(BufferMapOption option, size_t offset, size_t size) const noexcept override {
        if (size == 0) {
            return nullptr;
        }
        GLenum access = 0;
        switch (option) {
        case BufferMapOption::WRITE_DISCARD: {
            access = GL_MAP_WRITE_BIT;
            if (offset == 0 && size == this->size){
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
        return ptr;
    };

    virtual void unmap() const noexcept override {
        if (size != 0) {
            glUnmapNamedBuffer(id);
        }
    };

    // bind
    virtual void bind_uniform(uint32_t binding) const noexcept override {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }

    virtual ~GLBuffer() { glDeleteBuffers(1, &id); }

protected:
    GLuint id;

    virtual void _set_debug_label(const std::string &name) const noexcept override {
        glObjectLabel(GL_BUFFER, id, name.size(), name.data());
    }
};

} // namespace Graphics
} // namespace Goonya