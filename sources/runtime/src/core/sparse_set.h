#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "runtime/GAssert.h"

namespace Goonya {

template <typename T>
struct Handle {
    uint32_t index = std::numeric_limits<uint32_t>::max();   // sparse索引
    uint32_t version = std::numeric_limits<uint32_t>::max(); // 版本号
};
template <typename T>
class SparseSet {
private:
    struct SparseIndex {
        size_t index;
        uint32_t version;
    };

    std::vector<T> dense;                  // 紧凑存储实际数据
    std::vector<uint32_t> dense_to_sparse; // dense到sparse的映射
    std::vector<SparseIndex> sparse;       // 稀疏索引，大小 >= 最大键值

    std::vector<size_t> free_list; // 空闲key列表

    constexpr static size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

public:
    Handle<T> insert(const T &value) noexcept {
        size_t key;

        // 从空闲列表获取key，或分配新的
        if (!free_list.empty()) {
            key = free_list.back();
            free_list.pop_back();
        } else {
            key = sparse.size();
            sparse.emplace_back(0, 0);
        }

        size_t dense_index = dense.size();
        dense.push_back(value);
        dense_to_sparse.push_back(key);

        // 建立映射
        sparse[key].index = dense_index;

        return {(uint32_t)key, sparse[key].version};
    }

    template <typename... Args>
        requires std::is_constructible_v<T, Args...>
    Handle<T> emplace(Args &&...args) noexcept {
        size_t key;

        // 从空闲列表获取key，或分配新的
        if (!free_list.empty()) {
            key = free_list.back();
            free_list.pop_back();
        } else {
            key = sparse.size();
            sparse.emplace_back(0, 0);
        }

        size_t dense_index = dense.size();
        dense.emplace_back(std::forward<Args>(args)...);
        dense_to_sparse.push_back(key);

        // 建立映射
        sparse[key].index = dense_index;

        return {(uint32_t)key, sparse[key].version};
    }

    bool contains(const Handle<T> handle) const noexcept {
        uint32_t key = handle.index;

        // 检查key是否有效
        if (key >= sparse.size()) return false;
        if (sparse[key].index == INVALID_INDEX) return false;

        // 检查版本号是否匹配
        if (sparse[key].version != handle.version) return false;

        // 检查反向映射一致性
        size_t dense_index = sparse[key].index;
        return dense_index < dense.size() && dense_to_sparse[dense_index] == key;
    }

    T &operator[](const Handle<T> handle) noexcept {
        GN_ASSERT(contains(handle));
        return dense[sparse[handle.index].index];
    }

    const T &operator[](const Handle<T> handle) const noexcept {
        GN_ASSERT(contains(handle));
        return dense[sparse[handle.index].index];
    }

    T *get_or_null(const Handle<T> handle) noexcept {
        if (!contains(handle)) return nullptr;
        return &dense[sparse[handle.index].index];
    }

    const T *get_or_null(const Handle<T> handle) const noexcept {
        if (!contains(handle)) return nullptr;
        return &dense[sparse[handle.index].index];
    }

    void remove(const Handle<T> &handle) noexcept {
        if (!contains(handle)) return;

        uint32_t key = handle.index;
        size_t dense_index = sparse[key].index;
        size_t last_dense_index = dense.size() - 1;

        if (dense_index != last_dense_index) {
            // 将要删除的元素与最后一个元素交换
            std::ranges::swap(dense[dense_index], dense[last_dense_index]);

            // 更新被交换元素的映射
            uint32_t swapped_key = dense_to_sparse[last_dense_index];
            sparse[swapped_key].index = dense_index;
            dense_to_sparse[dense_index] = swapped_key;
        }

        // 删除最后一个元素（现在是要删除的元素）
        dense.pop_back();
        dense_to_sparse.pop_back();

        // 标记key为空闲
        sparse[key].index = INVALID_INDEX;
        // 版本号递增
        sparse[key].version++;

        // 将key加入空闲列表（可选）
        free_list.push_back(key);
    }

    void clear() noexcept {
        for (size_t key : dense_to_sparse) {
            if (sparse[key].index != INVALID_INDEX) {
                free_list.push_back(key);
                sparse[key].index = INVALID_INDEX;
            }
        }
        dense.clear();
        dense_to_sparse.clear();
    }

    size_t size() const noexcept { return dense.size(); }
    bool empty() const noexcept { return dense.empty(); }

    // 转发迭代器
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    iterator begin() noexcept { return dense.begin(); }
    iterator end() noexcept { return dense.end(); }

    const_iterator begin() const noexcept { return dense.begin(); }
    const_iterator end() const noexcept { return dense.end(); }

    const_iterator cbegin() const noexcept { return dense.cbegin(); }
    const_iterator cend() const noexcept { return dense.cend(); }
};
} // namespace Goonya