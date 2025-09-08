#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <format>

namespace Craft {

struct ResourceLocation{
    std::string name_space;
    std::string key;

    ResourceLocation(std::string name_space, std::string key) noexcept : name_space(std::move(name_space)), key(std::move(key)) {}
    static ResourceLocation parse(std::string_view full) noexcept {
        auto index = full.find(':');
        if (index == std::string::npos){
            return {"minecraft", std::string(full)};
        } else {
            return {std::string(full.substr(0, index)), std::string(full.substr(index + 1))};
        }
    }

    bool operator==(const ResourceLocation& rhs) const noexcept = default;
};

}

template <>
struct std::formatter<Craft::ResourceLocation> {
    static constexpr auto parse(std::format_parse_context &context) { return context.begin(); }
    template <typename FormatContext>
    auto format(const Craft::ResourceLocation& location, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}:{}", location.name_space, location.key);
    }
};

template<>
struct std::hash<Craft::ResourceLocation> {
    size_t operator()(const Craft::ResourceLocation& location) const noexcept{
        return (std::hash<std::string>{}(location.name_space) << 4) ^ std::hash<std::string>{}(location.key);
    }
};
