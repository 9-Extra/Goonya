#pragma once

#include "blockstate.h"
#include "core/cgmath/vector.h"
#include "craft/core/core.h"
#include "craft/core/registry.h"

#include <cassert>
#include <initializer_list>
#include <memory>
#include <vector>

namespace Craft {

extern Registry<Block> REGISTRY_BLOCK; // at all_blocks.cpp

class Block {
    friend class BlockStateMap;

private:
    // 重载create_default_blockstate()以自定义默认状态
    std::string display_name;

    std::vector<BlockStateProperty *> valied_properties;
    std::vector<std::unique_ptr<BlockState>> possible_states;
    BlockState *default_blockstate = nullptr;

    bool can_occlude;

public:
    Block(Block &) = delete;
    Block(Block &&) = delete;
    explicit Block(std::string display_name, bool can_occlude = true,
                   std::initializer_list<BlockStateProperty *> valied_properties = {})
        : display_name(std::move(display_name)), valied_properties(valied_properties), can_occlude(can_occlude) {
        build_blockstates();
        register_default_blockstate(possible_states[0].get());
    }
    virtual ~Block() = default;

    std::string_view get_display_name() const noexcept { return display_name; }
    const std::vector<std::unique_ptr<BlockState>> &get_possible_states() const noexcept { return possible_states; }
    BlockState *get_default_blockstate() const noexcept { return default_blockstate; }

    /**
     * @brief 给方块染色，需要blockstate文件中设置了对应的tintindex
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @param tintindex 染色索引，需要blockstate文件中设置了对应的tintindex
     * @note 需要时请重载此函数
     * @return Goonya::Vector3f 染色颜色
     */
    virtual Goonya::Vector3f get_tint_color(BlockState *state, BlockPos pos, int32_t tintindex) const noexcept {
        assert(false); // 重载它！！！
        return {};
    }

protected:
    void register_default_blockstate(BlockState* state) noexcept { 
        assert(state && state->block == this);
        default_blockstate = state; 
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