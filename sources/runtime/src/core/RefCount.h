#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

class RefCount {
private:
    mutable std::atomic<uint32_t> ref_count;

public:
    RefCount() noexcept {
        ref_count.store(0, std::memory_order::release);
    };
    virtual ~RefCount() = default;
    uint32_t get_ref_count() const noexcept { return ref_count.load(std::memory_order::consume); }

private:
    template <typename T>
    friend class Ref;

    void add_ref() noexcept { ref_count.fetch_add(1, std::memory_order::relaxed); }
    // 返回release后的引用计数
    uint32_t release() noexcept { return ref_count.fetch_sub(1, std::memory_order::acq_rel) - 1; }
};

template <typename T> // requires std::derived_from<RefCount, T> 防止在未定义完成时检查出错
class Ref final {
    static_assert(std::derived_from<T, RefCount>);

private:
    T *ptr;

public:
    Ref() noexcept : ptr(nullptr) {}
    Ref(T *ptr) noexcept // NOLINT
        : ptr(ptr) {
        if (ptr) {
            ptr->add_ref();
        }
    }
    Ref(const Ref<T> &other) noexcept
        : ptr(other.ptr) {
        if (ptr) {
            ptr->add_ref();
        }
    }
    Ref(Ref<T> &&other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
    ~Ref() {
        reset();
    }
    
    // cast
    template <std::derived_from<T> U>
    Ref(const Ref<U> &other) noexcept // NOLINT，向基类可以隐式转换
        : ptr(other.ptr) {
        if (ptr != nullptr) {
            ptr->add_ref();
        }
    }

    Ref<T>& operator=(const Ref<T>& other) noexcept { // NOLINT: self-assignment handled
        if (this->ptr == other.ptr){
            return *this;
        }
        reset();
        ptr = other.ptr;
        if (ptr){
            ptr->add_ref();
        }
        return *this;
    }

    bool operator==(const Ref<T>& other) const noexcept{
        return this->ptr == other.ptr;
    }

    const T *get() const noexcept { return ptr; }
    T *get() noexcept { return ptr; }
    const T *operator->() const noexcept { return ptr; }
    T *operator->() noexcept { return ptr; }

    explicit operator bool() const noexcept { return ptr != nullptr; }

    void reset() noexcept {
        if (ptr != nullptr) {
            if (ptr->release() == 0) {
                delete ptr; // 这里假定RefCount一定是new出来的
            }
            ptr = nullptr;
        }
    }

    template<typename U>
    static Ref<T> cast_from(const Ref<U>& src){
        return Ref<T>{dynamic_cast<T*>(const_cast<U*>(src.get()))};
    }
};

template <std::derived_from<RefCount> T, typename... Args>
    requires std::is_constructible_v<T, Args...>
Ref<T> create_ref(Args... args) {
    return Ref<T>{new T(std::forward<Args>(args)...)};
}

template<typename T>
struct std::hash<Ref<T>>{ // NOLINT
    size_t operator()(const Ref<T>& ref) const noexcept{
        return ref.get();
    }
};