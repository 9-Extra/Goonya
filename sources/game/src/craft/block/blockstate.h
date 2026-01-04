#pragma once

#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "craft/core/core.h"
#include "craft/core/registry.h"

#include <algorithm>

#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <memory>
#include <optional>
#include <rfl/enums.hpp>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Craft {

using BlockStatePropertyValue = uint32_t;

/**
 * @brief 表示一个BlockState的可用属性
 * 仅仅表示此属性的类型信息和名称，其中不包含属性的值。其值在BlockState中使用properties哈希表保存，类型擦除为BlockStatePropertyValue
 */
class BlockStateProperty final {
private:
    std::string name;
    std::type_index type; // 包含一个const std::type_info*，记录此Property的实际类型，只能是bool,
                          // BlockStatePropertyValue(uint32_t)或者使用其作为基础类型的枚举

    std::unordered_map<BlockStatePropertyValue, std::string> value_to_name;
    std::unordered_map<std::string, BlockStatePropertyValue, ::Goonya::StringHash, ::Goonya::StringEqual> name_to_value;

public:
    BlockStateProperty(BlockStateProperty &) = delete;
    BlockStateProperty(BlockStateProperty &&) = default;

    std::string_view get_name() const noexcept { return name; }
    std::type_index get_type() const noexcept { return type; }
    // 用于枚举可能的value
    const std::unordered_map<std::string, BlockStatePropertyValue, ::Goonya::StringHash, ::Goonya::StringEqual>& get_possible_value_and_names() const noexcept {
        return name_to_value;
    }

    bool validate(BlockStatePropertyValue value) const noexcept { return value_to_name.contains(value); }
    std::string_view to_string(BlockStatePropertyValue value) const noexcept {
        GN_ASSERT(validate(value));
        return value_to_name.at(value);
    }
    std::optional<BlockStatePropertyValue> from_string(std::string_view name) const noexcept{
        if (auto iter = name_to_value.find(name);iter != name_to_value.end()){
            return iter->second;
        } else {
            return std::nullopt;
        }
    }

    static std::unique_ptr<BlockStateProperty> create_bool(std::string name) {
        std::unique_ptr<BlockStateProperty> p{new BlockStateProperty{std::move(name), typeid(bool)}};
        BlockStatePropertyValue False{false};
        BlockStatePropertyValue True{true};

        p->value_to_name.emplace(False, "false");
        p->value_to_name.emplace(True, "true");
        p->name_to_value.emplace("false", False);
        p->name_to_value.emplace("true", True);
        return p;
    }
    static std::unique_ptr<BlockStateProperty> create_int(std::string name,
                                                          std::initializer_list<BlockStatePropertyValue> values) {
        std::unique_ptr<BlockStateProperty> p{new BlockStateProperty{std::move(name), typeid(BlockStatePropertyValue)}};
        for (BlockStatePropertyValue v : values) {
            std::string name = std::format("{}", v);
            p->value_to_name.emplace(v, name);
            p->name_to_value.emplace(name, v);
        }
        return p;
    }

    template <typename T>
        requires std::is_scoped_enum_v<T>
    static std::unique_ptr<BlockStateProperty> create_enum(std::string name) {
        std::unique_ptr<BlockStateProperty> p{new BlockStateProperty{std::move(name), typeid(T)}};
        for (auto [enum_name, value] : rfl::get_underlying_enumerator_array<T>()) {
            GN_ASSERT(value <= (intmax_t)std::numeric_limits<BlockStatePropertyValue>::max() && value >= (intmax_t)std::numeric_limits<BlockStatePropertyValue>::min());
            std::string name;
            if constexpr(std::formattable<T, char>){
                name = std::format("{}", T(value));
            } else {
                name = enum_name;
            }

            p->value_to_name.emplace(value, name);
            p->name_to_value.emplace(name, value);
        }
        return p;
    }

private:
    explicit BlockStateProperty(std::string name, const std::type_info &type_info)
        : name(std::move(name)), type(type_info) {}
};

extern Registry<BlockStateProperty> PROPERTY_REGISTRY;

class Block;
class BlockState final {
private:
    struct Hasher {
        size_t operator()(const std::tuple<BlockStateProperty *, BlockStatePropertyValue> key) const noexcept {
            return ((size_t)std::get<0>(key) << 2) & std::get<1>(key) ;
        }
    };

    Block *block = nullptr;
    // 因为属性应该不会太多，所以就不用哈希表了，直接vector
    std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>> properties;

    // 将当前的一个BlockStateProperty *改成BlockStatePropertyValue后变成的BlockState*，用于“修改”BlockState
    std::unordered_map<std::tuple<BlockStateProperty *, BlockStatePropertyValue>, BlockState *, Hasher> neighbors;

    bool can_occlude = true;
public:
    explicit operator bool() const noexcept { return block != nullptr; }

    Block *get_block() const noexcept { return block; }
    const std::vector<std::tuple<BlockStateProperty *, BlockStatePropertyValue>>& get_properties() const noexcept{
        return properties;
    }
    bool has_property(BlockStateProperty *property) const noexcept { 
        return std::ranges::any_of(properties, [=](auto iter){return property == std::get<0>(iter);});
    }
    BlockStatePropertyValue get_property_raw_value(BlockStateProperty *property) const noexcept {
        auto iter = std::ranges::find_if(properties, [=](auto iter){return property == std::get<0>(iter);});
        GN_ASSERT(iter != properties.end()); // 属性不存在！
        return std::get<1>(*iter);
    }
    bool get_property_bool(BlockStateProperty *property) const noexcept {
        if (property->get_type() != typeid(bool)) {
            LOG_ERROR("属性类型不匹配，尝试获取{}，实为{}", typeid(bool).name(), property->get_type().name());
            return false;
        } else {
            return (bool)get_property_raw_value(property);
        }
    }
    BlockStatePropertyValue get_property_int(BlockStateProperty *property) const noexcept {
        if (property->get_type() != typeid(BlockStatePropertyValue)) {
            LOG_ERROR("属性类型不匹配，尝试获取{}，实为{}", typeid(bool).name(), property->get_type().name());
            return 0;
        } else {
            return get_property_raw_value(property);
        }
    }
    std::string_view get_property_to_string(BlockStateProperty *property) const noexcept{
        return property->to_string(get_property_raw_value(property));
    }

    template <typename T>
        requires std::is_scoped_enum_v<T>
    T get_property_enum(BlockStateProperty *property) const noexcept {
        if (property->get_type() != typeid(T)) {
            LOG_ERROR("属性类型不匹配，尝试获取{}，实为{}", typeid(T).name(), property->get_type().name());
            return (T)0;
        } else {
            return (T)get_property_raw_value(property);
        }
    }

    // "修改"属性，实际上是返回修改后对应的blockstate实例指针
    BlockState* set_property(BlockStateProperty* property, BlockStatePropertyValue value) const noexcept {
        if (auto iter = neighbors.find(std::make_tuple(property, value));iter != neighbors.end()){
            return iter->second;
        } else {
            LOG_WARN("错误的属性修改，试图将{}修改为{}", property->get_name(), value);
            return nullptr;
        }
    }

    template <typename T> requires std::is_scoped_enum_v<T> || std::is_same_v<T, bool>
    BlockState* set_property(BlockStateProperty* property, T value) const noexcept{
        return set_property(property, (BlockStatePropertyValue)value);
    }

    bool can_hide_face(Direction direction) const noexcept{
        return can_occlude;
    }

private:
    friend class Block;
    BlockState() = default; // 返回空的blockstate
};
} // namespace Craft