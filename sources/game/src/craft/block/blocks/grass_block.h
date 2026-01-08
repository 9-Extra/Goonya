#pragma once

#include "craft/block/all_blockstate_properties.h"
#include "craft/block/block.h"

namespace Craft {
class GrassBlock : public Block {
public:
    explicit GrassBlock(std::string display_name, bool can_occlude = true,
                        std::initializer_list<BlockStateProperty *> valied_properties = {})
        : Block(std::move(display_name), can_occlude, valied_properties) {
        register_default_blockstate(get_possible_states()[0].get()->set_property(Properties::get().SNOWY, false));
    }

    Goonya::Vector3f get_tint_color(BlockState *state, BlockPos pos, int32_t tintindex) const noexcept override {
        GN_ASSERT(tintindex == 0); // 应该只有0
        return {0.5f, 0.7f, 0.2f}; // 理论上应该向当前Level请求此位置上草的颜色
    }
};

} // namespace Craft