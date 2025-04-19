#pragma once

#include <cstddef>
#include <functional>
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

} // namespace Goonya