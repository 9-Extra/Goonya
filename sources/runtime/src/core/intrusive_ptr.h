#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

template<class T> 
class intrusive_ptr {
public:
    intrusive_ptr() noexcept: px(nullptr){}

    intrusive_ptr(T* p, bool add_ref = true) noexcept: px(p) {
        if (px != nullptr && add_ref) {
            intrusive_ptr_add_ref(px);
        }
    }

    intrusive_ptr(const intrusive_ptr<T>& other) noexcept{
        px = other.px;
        if (other){
            intrusive_ptr_add_ref(px);
        }
    }

    intrusive_ptr<T>& operator=(const intrusive_ptr<T>& other) noexcept{
        if (px != nullptr) {
            intrusive_ptr_release(px);
        }
        px = other.px;
        if (px != nullptr){
            intrusive_ptr_add_ref(px);
        }
        return *this;
    }

    intrusive_ptr(intrusive_ptr<T>&& other) noexcept{
        px = other.px;
        other.px = nullptr;
    }


    template<class D> requires std::derived_from<D, T>
    intrusive_ptr(intrusive_ptr<D> p) noexcept : px(p.get()) {
        intrusive_ptr_add_ref(p.get());
    }

    template<class... Args> requires std::is_constructible_v<T, Args...>
    intrusive_ptr(Args... args) noexcept
    : px(new T(std::forward<Args>(args)...))
    {
        intrusive_ptr_add_ref(px);
    }
    std::size_t hash() const noexcept{
        return (std::size_t)px;
    }

    T* get() noexcept{
        return px;
    }

    T* const get() const noexcept{
        return px;
    }

    T& operator*() noexcept{
        return *px;
    }

    T* operator->() noexcept{
        return px;
    }

    T* const operator->() const noexcept{
        return px;
    }

    explicit operator bool() const noexcept{
        return px != nullptr;
    }

    ~intrusive_ptr() {
        if (px != nullptr) {
            intrusive_ptr_release(px);
        }
    }
private:
    T * px;
};

template<class T>
class intrusive_ptr_base {
public:
    intrusive_ptr_base() : ref_count(0) {}

    friend void intrusive_ptr_add_ref(intrusive_ptr_base<T> const* p) noexcept{
        p->ref_count++;
    }

    friend void intrusive_ptr_release(intrusive_ptr_base<T> const* p) {
        p->ref_count--;
        if (p->ref_count == 0) {
            delete static_cast<T const*>(p);
        }
    }

    intrusive_ptr<T> self() noexcept{
        return intrusive_ptr<T>((T*)this);
    }

private:
    mutable uint32_t ref_count;
};