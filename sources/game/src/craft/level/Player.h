#pragma once

#include "core/cgmath.h"
#include "craft/core/core.h"
#include "function/world/GObject.h"
#include <cassert>
namespace Craft {

struct Player {
    std::shared_ptr<Goonya::GObject> player;
    ChunkPos last_chunk_pos;
    int32_t chunk_load_distance = -1;

    explicit Player(const std::shared_ptr<Goonya::GObject> &player) : player(player) {
        assert(player);
        last_chunk_pos = ChunkPos{BlockPos{get_position()}};
    }

    Goonya::Vector3f get_position() const noexcept {
        return player->get_transform().position;
    }

    Goonya::Vector3f get_direction() const noexcept {
        return player->get_transform().forward_direction();
    }
};

} // namespace Craft