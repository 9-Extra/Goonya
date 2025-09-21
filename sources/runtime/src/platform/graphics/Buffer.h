#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include <cassert>
#include <span>

namespace Goonya::Graphics {

enum class BufferType { STATIC, DYNAMIC, STREAM, READBACK };
enum class BufferMapOption {
    WRITE_DISCARD, // 丢弃旧数据，避免复制
    WRITE_MODIFY,  // 修改旧数据，可能导致从显存到内存的复制
    READ_ONLY,     // 用于读
    READ_WRITE,    // 用于读写
};

class Buffer : public intrusive_ptr_base<Buffer> {
protected:
    Buffer(size_t size, BufferType type) : size(size), type(type) {}

    virtual void _set_debug_label(const std::string &name) const noexcept = 0;

    size_t size;
    BufferType type;

public:
    size_t get_size() const noexcept { return size; }
    BufferType get_type() const noexcept { return type; }

    virtual void write(std::span<const std::byte> data, size_t offset) = 0;
    virtual void *map(BufferMapOption option) const noexcept = 0;
    virtual void *map_range(BufferMapOption option, size_t offset, size_t size) const noexcept = 0;
    virtual void unmap() const noexcept = 0;

    virtual void invalidate() noexcept = 0;

    virtual void bind_uniform(uint32_t binding) const noexcept = 0;
    virtual void bind_uniform_ranged(uint32_t binding, size_t offset, size_t size) const noexcept = 0;

    virtual void bind_storage(uint32_t binding) const noexcept = 0;
    virtual void bind_storage_ranged(uint32_t binding, size_t offset, size_t size) const noexcept = 0;

    void set_debug_label(const std::string &name) const noexcept {
#ifdef DEBUG
        _set_debug_label(name);
#endif
    }
    virtual ~Buffer() = default;
};

// 使用c++定义的结构体内存布局进行写入
template <class T>
class StructBufferWriter {
private:
    Buffer *buffer;
    T *ptr;

public:
    StructBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option)
        : buffer(buffer.get()), ptr(static_cast<T *>(buffer->map(option))) {
        assert(buffer);
    }
    StructBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option, size_t offset)
        : buffer(buffer.get()), ptr(static_cast<T *>(buffer->map_range(option, offset, sizeof(T)))) {
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
    Buffer *buffer;
    T *ptr;
    size_t element_count;
public:
    // 映射整个缓冲区
    ArrayBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option)
        : buffer(buffer.get()), element_count(buffer->get_size() / sizeof(T)) {
        assert(buffer);
        ptr = static_cast<T *>(buffer->map(option));
    }

    // 映射指定范围的缓冲区
    ArrayBufferWriter(intrusive_ptr<Buffer> buffer, BufferMapOption option, size_t start_index, size_t element_count)
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
    Buffer *buffer;

public:
    DynamicBufferWriter(intrusive_ptr<Buffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option)
        : Meta::DynamicStructWriter(layout, buffer->map(option)), buffer(buffer.get()) {
        assert(buffer);
    }
    DynamicBufferWriter(intrusive_ptr<Buffer> buffer, const Meta::LayoutInfo &layout, BufferMapOption option,
                        size_t offset)
        : Meta::DynamicStructWriter(layout, buffer->map_range(option, offset, layout.size)), buffer(buffer.get()) {
        assert(buffer);
    }
    DynamicBufferWriter(DynamicBufferWriter &other) = delete;

    ~DynamicBufferWriter() { buffer->unmap(); }
};

} // namespace Goonya::Graphics
