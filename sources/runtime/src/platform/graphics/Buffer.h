#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

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
public:
    virtual void bind_indices() const noexcept = 0;
    virtual uint32_t get_index_count() const noexcept = 0;
};

class UniformBuffer: public Buffer{
public:
    virtual void bind_uniform(uint32_t binding) const noexcept = 0;    
};

struct VertexLayout{
    std::vector<std::tuple<uint32_t, std::string, Meta::FieldType, size_t>> attributes;
    size_t size;
};  

class VertexBuffer: public Buffer{
public:
    virtual void bind_vertices() const = 0;
    const VertexLayout get_vertex_layout() const noexcept{
        return vertex_layout;
    }
protected:
    VertexLayout vertex_layout;
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