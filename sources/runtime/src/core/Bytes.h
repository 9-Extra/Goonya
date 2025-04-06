#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace Goonya {

class Bytes {
public:
    explicit Bytes(size_t size) : size(size) { ptr = new uint8_t[size]; }
    explicit Bytes(size_t size, const void* data) : Bytes(size) {
        memcpy(ptr, data, size);
    }
    Bytes(const Bytes&) = delete;
    Bytes(Bytes&& that) {
        this->size = that.size;
        this->ptr = that.ptr;
        that.ptr = nullptr;
    }

    template<typename T>
    static Bytes from_span(std::span<T> span) {
        return Bytes{span.size_bytes(), span.data()};
    }

    ~Bytes() { delete[] ptr; }

    size_t get_size() const noexcept { return size; }

    Bytes clone() const noexcept{
        return Bytes{size, ptr};
    }

    const uint8_t *data() const noexcept { return ptr; }
    uint8_t *data() noexcept { return ptr; }

    template<typename T>
    std::span<T> as_span(size_t offset = 0) noexcept{
        return std::span<T>((T*)(ptr + offset), (size - offset) / sizeof(T));
    }

    template<typename T>
    std::span<const T> as_span(size_t offset = 0) const noexcept{
        return std::span<const T>((const T*)(ptr + offset), (size - offset) / sizeof(T));
    }

private:
    uint8_t *ptr;
    size_t size;
};

} // namespace Goonya