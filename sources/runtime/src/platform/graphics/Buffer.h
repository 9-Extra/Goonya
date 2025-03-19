#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>

namespace Goonya {
namespace Graphics {

enum class BufferType { STATIC, DYNAMIC, STREAM, READBACK };

class Buffer : public intrusive_ptr_base<Buffer> {
public:
    virtual ~Buffer() = default;

    size_t get_size() const noexcept { return size; }
    BufferType get_type() const noexcept { return type; }

    virtual void write(const std::span<uint8_t> data, size_t offset = 0) = 0;
    virtual void *map() const noexcept = 0;
    virtual void unmap() const noexcept = 0;
    
    virtual void bind_uniform(uint32_t binding) const noexcept = 0;

    protected:
    Buffer(size_t size, BufferType type): size(size), type(type) {}

    size_t size;
    BufferType type;
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBufferWriter {
public:
    StructBufferWriter(intrusive_ptr<Buffer> buffer) : buffer(buffer), ptr((T *)(buffer->map())) {}
    StructBufferWriter(StructBufferWriter &other) = delete;

    T *operator->() noexcept { return ptr; }

    ~StructBufferWriter() noexcept { buffer->unmap(); }

private:
    intrusive_ptr<Buffer> buffer;
    T *ptr;
};

class DynamicBufferWriter : public Meta::DynamicStructWriter {
public:
    DynamicBufferWriter(intrusive_ptr<Buffer> buffer, const Meta::LayoutInfo &layout)
        : Meta::DynamicStructWriter(layout, buffer->map()), buffer(buffer) {}
    DynamicBufferWriter(DynamicBufferWriter &other) = delete;

    ~DynamicBufferWriter() { buffer->unmap(); }

private:
    intrusive_ptr<Buffer> buffer;
};

} // namespace Graphics
} // namespace Goonya