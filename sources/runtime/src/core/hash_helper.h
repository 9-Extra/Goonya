#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace Goonya {

template <typename T>
inline void hash_combine(size_t &seed, const T &v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename... ARGS>
inline void hash_combine(size_t &seed, const ARGS &...v) {
    hash_combine(v...);
}

template <typename... TT>
size_t hash_tuple(const std::tuple<TT...> &tt) {
    size_t seed = 0;
    std::apply([&seed](auto &&...x) { ((hash_combine(seed, x)), ...); }, tt);
    return seed;
}

// 为了支持使用std::string_view查找使用std::string作为键的哈希表，需要支持异构查找
struct StringHash {
    using is_transparent = void; // 启用透明哈希的关键
    size_t operator()(std::string_view str) const noexcept { return std::hash<std::string_view>{}(str); }
};

// 用来支持std::unordered_map<std::string, T, StringHash, StringEqual>
struct StringEqual {
    using is_transparent = void; // 启用透明比较的关键
    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs == rhs; }
};

// 用来支持std::unordered_map<std::unique_ptr<T>, XXX, PointerHash, PointerHash>
struct PointerHash {
    using is_transparent = void; // 启用透明哈希的关键
    template <typename T>
    size_t operator()(const T &ptr) const noexcept {
        return std::hash<const void *>{}(get_pointer(ptr));
    }
private:
    template <typename T>
    static void *get_pointer(const T &ptr) noexcept {
        if constexpr (std::is_pointer_v<T>) {
            return ptr;
        } else {
            return ptr.get(); // 智能指针
        }
    }
};

struct PointerEqual {
    using is_transparent = void; // 启用透明比较的关键
    template <typename A, typename B>
    bool operator()(const A& lhs, const B& rhs) const noexcept {
        return get_pointer(lhs) == get_pointer(rhs);
    }

private:
    template <typename T>
    static void *get_pointer(const T &ptr) noexcept {
        if constexpr (std::is_pointer_v<T>) {
            return ptr;
        } else {
            return ptr.get(); // 智能指针
        }
    }
};

} // namespace Goonya