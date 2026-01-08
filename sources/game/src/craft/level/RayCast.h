#pragma once

#include "core/cgmath/vector.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"

namespace Craft {

struct Ray {
    Goonya::Vector3f origin;
    Goonya::Vector3f direction;
};

struct BlockHitResult {
    BlockPos position{};
    Direction normal{};
    BlockState *block_state = nullptr;

    explicit operator bool() const noexcept { return block_state != nullptr; }
};

} // namespace Craft