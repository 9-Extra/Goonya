#pragma once

#include "blockstate.h"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Craft{

class BlockStateMap{
private:
    static std::vector<BlockState*> blockstates; // BlockState所有权在Block势力中，这里是引用
    static std::unordered_map<BlockState*, uint32_t> blockstate_to_id;
public:
    BlockStateMap() = delete;
    static void initalize();

    static const std::vector<BlockState*>& get_blockstates() noexcept{
        return blockstates;
    }

    static BlockState* find_blockstate(uint32_t id) noexcept{
        return blockstates.at(id);
    }

private:
};



}