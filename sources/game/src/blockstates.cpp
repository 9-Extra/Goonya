#include "blockstates.h"

#include "block.h"
#include "blockstate.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <ranges>
#include <vector>

namespace Craft {

std::vector<BlockState*> BlockStateMap::blockstates;
std::unordered_map<BlockState *, uint32_t> BlockStateMap::blockstate_to_id;

void BlockStateMap::initalize() {
    assert(blockstates.size() == 0);           // 不要反复初始化
    assert(REGISTRY_BLOCK.entry_count() != 0); // 必须在Blocks初始化之后

    for (const Block *const_block : std::views::keys(REGISTRY_BLOCK)) {
        for(const std::unique_ptr<BlockState>& state: const_block->get_possible_states()){
            BlockState* s = state.get();
            uint32_t id = blockstates.size();

            blockstates.push_back(s);
            blockstate_to_id.emplace(s, id);
        }
    }
    // 此时所有blockstate已经生成完毕
}
} // namespace Craft