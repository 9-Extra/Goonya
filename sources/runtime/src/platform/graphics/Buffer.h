#pragma once

#include "core/intrusive_ptr.h"
#include <cstdint>
#include <span>

#include "resource/resources.h"

namespace Goonya {
namespace Graphics {

enum BufferType{
    STATIC,
    DYNAMIC,
    READBACK
};

class Buffer: public intrusive_ptr_base<Buffer>{
public:
    virtual ~Buffer() = default;

    virtual uint32_t get_size() const noexcept = 0;
    virtual BufferType get_type() const noexcept = 0;

    virtual void write(const std::span<uint8_t> data, uint32_t offset = 0) = 0;
    virtual void* map() const noexcept = 0;
    virtual void unmap() const noexcept = 0;
protected:
    Buffer() = default;
};

class IndexBuffer: public Buffer{
    virtual uint32_t get_index_count() const noexcept = 0;
};

class UniformBuffer: public Buffer{
public:
    virtual void bind_uniform(uint32_t binding) const noexcept = 0;    
};

class VertexBuffer: public Buffer{
};

// 使用c++定义的结构体内存布局进行写入
template <class T> class StructBufferWriter{
public:
    StructBufferWriter(intrusive_ptr<Buffer> buffer): buffer(buffer), ptr((T*)(buffer->map())) {}
    StructBufferWriter(StructBufferWriter& other) = delete;
    
    T* operator->() noexcept{
        return ptr;
    }

    ~StructBufferWriter() noexcept{
        buffer->unmap();
    }
private:
    intrusive_ptr<Buffer> buffer;
    T* ptr;
};

}
}