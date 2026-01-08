#pragma once

#include "block/all_blocks.h"
#include "block/all_blockstate_properties.h"
#include "block/blockstates.h"
#include "model_manager.h"

namespace Craft {

inline void initalize() {
    Properties::initalize();
    Blocks::initalize();
    BlockStateMap::initalize();
    ModelManager::initalize();
};

} // namespace Craft