#include "GLBuffer.h"

namespace Goonya {

std::byte *GLBuffer::map_range(BufferMapOption option, size_t offset, size_t size) const noexcept {
    GN_ASSERT((option == BufferMapOption::READ_ONLY && type == BufferType::READBACK) ||
              ((option == BufferMapOption::WRITE_DISCARD || option == BufferMapOption::WRITE_MODIFY) &&
               type == BufferType::MODIFIABLE));
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
    }
    void *ptr = glMapNamedBufferRange(id, offset, size, access);
    GN_ASSERT(ptr);
    return reinterpret_cast<std::byte *>(ptr);
};
void GLBuffer::write(std::span<const std::byte> data, BufferMapOption option, size_t offset) noexcept {
    GN_ASSERT(type == BufferType::MODIFIABLE);
    GN_ASSERT(data.size_bytes() + offset <= size && option != BufferMapOption::READ_ONLY);
    std::byte *ptr = map_range(option, offset, data.size_bytes());
    memcpy(ptr, data.data(), data.size_bytes());
    unmap();
};
} // namespace Goonya