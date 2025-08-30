#pragma once

#include "all_blockstate_properties.h"
#include "all_blocks.h"
#include "blockstates.h"
#include "model_manager.h"

namespace Craft{

inline void initalize(){
    Properties::initalize();
    Blocks::initalize();
    BlockStateMap::initalize();
    ModelManager::initalize();
};


}