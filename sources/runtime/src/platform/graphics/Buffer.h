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
enum class BufferMapOption { 
    WRITE_DISCARD, // 丢弃旧数据，避免复制
    WRITE_MODIFY, // 修改旧数据，可能导致从显存到内存的复制
    READ_ONLY, // 用于读
    READ_WRITE, // 用于读写
};

class Buffer : public intrusive_ptr_base<Buffer> {
public:
    virtual ~Buffer() = default;

    size_t get_size() const noexcept { return size; }
    BufferType get_type() const noexcept { return type; }

    virtual void write(const std::span<uint8_t> data, size_t offset = 0) = 0;
    virtual void *map(BufferMapOption option) const noexcept = 0;
    virtual void *map_range(BufferMapOption option, size_t offset, size_t size) const noexcept = 0;
    virtual void unmap() const noexcept = 0;

    virtual void bind_uniform(uint32_t binding) const noexcept = 0;

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        _set_debug_label(name);
#endif
    }

protected:
    Buffer(size_t size, BufferType type) : size(size), type(type) {}

    virtual void _set_debug_label(const std::string &name) const noexcept = 0;

    size_t size;
    BufferType type;
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBufferWriter {
public:
    StructBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option) : buffer(buffer.get()), ptr((T *)(buffer->map(option))) {
        assert(buffer);
    }
    StructBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option, size_t offset) : buffer(buffer.get()), ptr((T *)(buffer->map_range(option, offset, sizeof(T)))) {
        assert(buffer);
    }
    StructBufferWriter(StructBufferWriter &other) = delete;

    T *operator->() noexcept { return ptr; }

    ~StructBufferWriter() noexcept { buffer->unmap(); }

private:
    Buffer *buffer;
    T *ptr;
};

class DynamicBufferWriter : public Meta::DynamicStructWriter {
public:
    DynamicBufferWriter(intrusive_ptr<Buffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option)
        : Meta::DynamicStructWriter(layout, buffer->map(option)), buffer(buffer.get()) {
        assert(buffer);
    }
    DynamicBufferWriter(intrusive_ptr<Buffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option, size_t offset)
        : Meta::DynamicStructWriter(layout, buffer->map_range(option, offset, layout.size)), buffer(buffer.get()) {
        assert(buffer);
    }
    DynamicBufferWriter(DynamicBufferWriter &other) = delete;

    ~DynamicBufferWriter() { buffer->unmap(); }

private:
    Buffer *buffer;
};

} // namespace Graphics
} // namespace Goonya