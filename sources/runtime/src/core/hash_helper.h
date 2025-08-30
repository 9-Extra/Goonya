#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <tuple>

namespace Goonya {

template <typename T>
inline void hash_combine(size_t &seed, const T &v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename... ARGS>
inline void hash_combine(size_t &seed, const ARGS&... v) {
    hash_combine(v...);
}

template<typename... TT>
size_t hash_tuple(const std::tuple<TT...> &tt){
    size_t seed = 0;
    std::apply([&seed](auto &&...x) { ((hash_combine(seed, x)), ...); }, tt);
    return seed;
}

// 为了支持使用std::string_view查找使用std::string作为键的哈希表，需要支持异构查找
struct StringHash {
using is_transparent = void; // 启用透明哈希的关键
size_t operator()(std::string_view str) const noexcept {
    return std::hash<std::string_view>{}(str);
}
};

// 用来支持std::unordered_map<std::string, T, StringHash, StringEqual>
struct StringEqual {
using is_transparent = void; // 启用透明比较的关键
bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
    return lhs == rhs;
}

};

} // namespace Goonya