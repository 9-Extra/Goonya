#pragma once

#include "blockstate.h"
#include "craft/core/registry.h"

#include <cassert>
#include <initializer_list>
#include <memory>
#include <vector>

namespace Craft {

extern Registry<Block> REGISTRY_BLOCK; // at all_blocks.cpp

class Block {
private:
    std::string display_name;
    std::vector<BlockStateProperty *> valied_properties;

    // 现在初始化时生成
    BlockState *default_blockstate = nullptr;
    std::vector<std::unique_ptr<BlockState>> possible_states;

    friend class BlockStateMap;

public:
    Block(Block &) = delete;
    Block(Block &&) = delete;
    explicit Block(std::string display_name, std::initializer_list<BlockStateProperty *> valied_properties = {})
        : display_name(std::move(display_name)), valied_properties(valied_properties) {
        build_blockstates();
        this->default_blockstate = possible_states[0].get();
    }
    virtual ~Block() = default;

    std::string_view get_display_name() const noexcept { return display_name; }
    const std::vector<std::unique_ptr<BlockState>> &get_possible_states() const noexcept { return possible_states; }
    BlockState* get_default_blockstate() const noexcept{
        return default_blockstate;
    }

private:
    void build_blockstates();
};

} // namespace Craft

// 因为实现中需要Registry<Block> REGISTRY_BLOCK的实现，所以放在这里
template <>
struct std::formatter<Craft::Block> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::Block &block, FormatContext &ctx) const {
        return std::format_to(ctx.out(), "{}", Craft::REGISTRY_BLOCK.find_key(&block));
    }
};

template <>
struct std::formatter<Craft::BlockState> {
    constexpr auto parse(std::format_parse_context &context) { return context.begin(); } // NOLINT
    template <typename FormatContext>
    auto format(const Craft::BlockState &state, FormatContext &ctx) const {
        std::format_to(ctx.out(), "Block{{{}}} ", *state.get_block());
        std::format_to(ctx.out(), "[");
        bool is_first = true;
        for (const auto [property, value] : state.get_properties()) {
            if (is_first) {
                std::format_to(ctx.out(), "{} = {}", property->get_name(), property->to_string(value));
                is_first = false;
            } else {
                std::format_to(ctx.out(), ", {} = {}", property->get_name(), property->to_string(value));
            }
        }
        return std::format_to(ctx.out(), "]");
    }
};