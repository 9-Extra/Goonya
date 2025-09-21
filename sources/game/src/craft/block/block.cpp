#include "block.h"
#include "blockstate.h"
#include <cassert>
#include <memory>
#include <ranges>

namespace Craft{

Registry<Block> REGISTRY_BLOCK;

void Block::build_blockstates() {
    assert(this->valied_properties.size() < 255);
    uint8_t property_count = (uint8_t)this->valied_properties.size();

    if (property_count == 0) {
        // 捷径，对没有属性的block只有一个blockstate
        BlockState *s = new BlockState();
        s->block = this;
        s->can_occlude = can_occlude;
        // 填充此block的默认值
        possible_states.emplace_back(s);
        return;
    }

    // 对于持有一个以上属性的，生成此方块下所有可能的BlockState
    std::vector<std::vector<BlockStatePropertyValue>> property_value_cache; // 缓存所有property的可能值
    property_value_cache.reserve(property_count);
    std::vector<size_t> strides{1}; // 计算偏移，从1开始
    strides.reserve(property_count + 1);
    for (auto property : this->valied_properties) {
        property_value_cache.emplace_back(std::ranges::to<std::vector<BlockStatePropertyValue>>(
            std::views::values(property->get_possible_value_and_names())));
        size_t possible_count = property_value_cache.back().size();

        strides.push_back(strides.back() * possible_count);
    }

    assert(possible_states.empty());
    const size_t state_count = strides.back();
    possible_states.reserve(state_count);
    for (size_t i = 0; i < state_count; i++) {
        BlockState *s = new BlockState(); // 只能一个一个new出来
        s->block = this;
        s->can_occlude = can_occlude;
        possible_states.emplace_back(s); 
    }

    for (size_t i = 0; i < state_count; i++) {
        for (uint8_t p = 0; p < property_count; p++) {
            size_t p_bias = i % strides[p + 1] / strides[p]; // 此state对应的值是p属性中的第几个可能取值
            size_t remain_bias = i % strides[p];
            // 填充属性值
            BlockStateProperty *property = this->valied_properties[p];
            possible_states[i]->properties.emplace_back(property, property_value_cache[p][p_bias]);
            // 构建邻居表
            for (auto [j, value] : std::views::enumerate(property_value_cache[p])) {
                possible_states[i]->neighbors.emplace(std::make_tuple(property, value), possible_states[remain_bias + strides[p] * j].get());
            }
        }
    }

#if 0
    for (std::unique_ptr<BlockState>& s : possible_states) {
        LOG_WARN("{} count = {}", *s, s->neighbors.size());
        for (auto [key, n] : s->neighbors) {
            auto [p, v] = key;
            LOG_INFO("{:<10} -> {:<10}: {}", p->get_name(), p->to_string(v), *n);
        }
    }
#endif

}
} // namespace Craft