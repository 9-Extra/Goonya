#pragma once

#include "core/hash_helper.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Craft{

template<typename T>
class Registry{
public:
    using TID = uint32_t;
private:
    std::vector<std::unique_ptr<T>> entry_list; // 持有所有权
    std::unordered_map<const T*, TID> entry_to_id; 
    std::unordered_map<std::string, TID, ::Goonya::StringHash, ::Goonya::StringEqual> key_to_id;
    std::vector<std::string> id_to_key;
public:
    void do_register(std::string key, std::unique_ptr<T>&& entry){
        assert(entry && !entry_to_id.contains(entry.get())); // 重复注册
        TID id = entry_list.size();

        entry_to_id[entry.get()] = id;
        entry_list.push_back(std::move(entry));
        key_to_id.emplace(key, id);
        id_to_key.emplace_back(key);

    }

    uint32_t entry_count() const noexcept{
        return (uint32_t)entry_list.size();
    }
    bool contains(T* entry) const noexcept{
        return entry_to_id.contains(entry);
    }
    bool contains(std::string_view key) const noexcept{
        return key_to_id.contains(key);
    }

    std::string_view find_key(TID id) const noexcept {
        return id_to_key.size() > id ? id_to_key[id] : std::string_view();
    };
    std::string_view find_key(const T* entry) const noexcept {
        if (auto iter = entry_to_id.find(entry);iter != entry_to_id.end()){
            return id_to_key[iter->second];    
        } else {
            return {};
        }
    };
    T* find_entry(TID id) const noexcept {
        return id < entry_list.size() ? entry_list[id].get() : nullptr;
    };
    T* find_entry(std::string_view key) const noexcept {
        if (auto iter = key_to_id.find(key);iter != key_to_id.end()){
            return entry_list[iter->second].get();
        } else {
            return nullptr;
        }
    };

    // 转发迭代器，相当于迭代entry_to_id，只能是const，不希望block被修改（吗？）
    decltype(auto) begin() const noexcept {
        return entry_to_id.begin();
    }
    decltype(auto) end() const noexcept {
        return entry_to_id.end();
    }
};

}