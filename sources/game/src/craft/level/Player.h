#pragma once

#include "craft/core/core.h"
#include "function/world/GObject.h"


namespace Craft {

struct Player {
    std::shared_ptr<Goonya::GObject> player;
    ChunkPos last_chunk_pos;
    int32_t chunk_load_distance = -1;

    explicit Player(const std::shared_ptr<Goonya::GObject> &player) : player(player) {
        GN_ASSERT(player);
        last_chunk_pos = ChunkPos{BlockPos{get_position()}};
    }

    Goonya::Vector3f get_position() const noexcept {
        return player->get_local_transform().position;
    }

    Goonya::Vector3f get_direction() const noexcept {
        return player->get_local_transform().forward_direction();
    }
};

} // namespace Craft