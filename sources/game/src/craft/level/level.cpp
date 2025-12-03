#include "level.h"

#include "craft/block/all_blocks.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/Player.h"
#include "craft/level/RayCast.h"
#include "craft/level/chunk.h"
#include "function/world/World.h"
#include <cassert>

namespace Craft {

Level::Level(Goonya::World *world, const std::shared_ptr<Goonya::GObject> &player)
    : Goonya::TickFunction(Goonya::TickType::FIXED_TICK), bind_world(world), level_renderer(world->main_scene()),
      player(player) {
    assert(world != nullptr);
}

void Level::tick() {
    load_chunks();

    Goonya::Vector3f player_pos = player.get_position();
    Goonya::Vector3f player_dir = player.get_direction();
    BlockHitResult hit_result = ray_cast(Ray{player_pos, player_dir}, 64);

    if (hit_result) {
        BlockPos pos = BlockPos{hit_result.position + get_direction_vector(hit_result.normal)};
        set_block_state(pos, Blocks::get().GRANITE->get_default_blockstate());
    } else {
    }

    level_renderer.render_frame();
}

void Level::load_chunks() {
    // 玩家位置，假定玩家类一定在根节点上
    ChunkPos player_chunk_pos = ChunkPos{BlockPos{player.get_position()}};
    if (player_chunk_pos == player.last_chunk_pos && chunk_load_distance == player.chunk_load_distance) {
        return; // 玩家在同一个区块里且加载范围不变，则不需要加载新的区块
    }

    auto load_new_chunk = [&](ChunkPos pos) -> void {
        if (auto iter = accessible_chunk.find(pos); iter != accessible_chunk.end()) {
            level_renderer.register_chunk(iter->second);
            return;
        }

        if (auto iter = all_chunks.find(pos); iter != all_chunks.end()) {
            // 区块生成到一半继续等
        } else {
            Ref<Chunk> chunk = create_ref<Chunk>(pos);
            all_chunks.emplace(pos, chunk);
            chunk_generator.process_chunk_async(chunk, [level = Ref{this}](const Ref<Chunk> &chunk) mutable {
                level->accessible_chunk.emplace(chunk->chunk_pos, chunk);
                level->level_renderer.register_chunk(chunk);
            });
        }
    };

    auto drop_chunk = [&](ChunkPos pos) -> void { level_renderer.unregister_chunk(pos); };

    // intersect即使误判为true不会导致问题
    bool intersect =
        player.last_chunk_pos.is_in_region(player_chunk_pos, chunk_load_distance + player.chunk_load_distance);
    if (intersect) {
        for (ChunkPos pos : ChunkPos::iterate_region(player.last_chunk_pos - player.chunk_load_distance,
                                                     player.last_chunk_pos + player.chunk_load_distance + 1)) {
            if (pos.is_in_region(player_chunk_pos, chunk_load_distance)) {
                continue; // 跳过相交区域
            }
            drop_chunk(pos);
        }

        for (ChunkPos pos : ChunkPos::iterate_region(player_chunk_pos - chunk_load_distance,
                                                     player_chunk_pos + chunk_load_distance + 1)) {
            if (pos.is_in_region(player.last_chunk_pos, player.chunk_load_distance)) {
                continue; // 跳过相交区域
            }
            load_new_chunk(pos);
        }
    } else {
        for (ChunkPos pos : ChunkPos::iterate_region(player.last_chunk_pos - player.chunk_load_distance,
                                                     player.last_chunk_pos + player.chunk_load_distance + 1)) {
            drop_chunk(pos);
        }
        for (ChunkPos pos : ChunkPos::iterate_region(player_chunk_pos - chunk_load_distance,
                                                     player_chunk_pos + chunk_load_distance + 1)) {
            load_new_chunk(pos);
        }
    }

    player.last_chunk_pos = player_chunk_pos;
    player.chunk_load_distance = chunk_load_distance;
}

BlockHitResult Level::ray_cast(Ray ray, float max_distance) const noexcept {
    assert(max_distance >= 0);

    BlockPos current_pos = BlockPos(ray.origin);

    // 捷径，直接判断起点是否有方块
    if (BlockState *s = get_block_state(current_pos); s != nullptr) {
        if (s->get_block() != Blocks::get().AIR) {
            // 因为起点在方块内部，所以取绝对值最大的方向作为命中方向，则法方向与该方向相反
            Direction hit_dir;
            Goonya::Vector3f abs_dir = {std::abs(ray.direction.x), std::abs(ray.direction.y),
                                        std::abs(ray.direction.z)};
            if (abs_dir.x > abs_dir.y && abs_dir.x > abs_dir.z) {
                hit_dir = ray.direction.x > 0 ? Direction::WEST : Direction::EAST;
            } else if (abs_dir.y > abs_dir.z) {
                hit_dir = ray.direction.y > 0 ? Direction::DOWN : Direction::UP;
            } else {
                hit_dir = ray.direction.z > 0 ? Direction::NORTH : Direction::SOUTH;
            }
            return {current_pos, hit_dir, s};
        }
    }

    if (max_distance == 0.0f) {
        return {};
    }

    const Goonya::Vector3f arrow = ray.direction.normalize() * max_distance; // 从起点指向终点的向量
    const Vector3i arrow_sign = {Goonya::sign(arrow.x), Goonya::sign(arrow.y), Goonya::sign(arrow.z)};

    // 计算arrow的倒数的绝对值，即距离下一个方块每个方向增长的长度
    const Goonya::Vector3f step = {
        arrow_sign.x != 0 ? arrow_sign.x / arrow.x : std::numeric_limits<float>::max(),
        arrow_sign.y != 0 ? arrow_sign.y / arrow.y : std::numeric_limits<float>::max(),
        arrow_sign.z != 0 ? arrow_sign.z / arrow.z : std::numeric_limits<float>::max(),
    };
    // LOG_INFO("{}", step);
    //  取起点的小数部分
    const Goonya::Vector3f start_frac = {
        ray.origin.x - current_pos.x,
        ray.origin.y - current_pos.y,
        ray.origin.z - current_pos.z,
    };

    Goonya::Vector3f current = step * Goonya::Vector3f{arrow_sign.x > 0 ? 1 - start_frac.x : start_frac.x,
                                                       arrow_sign.y > 0 ? 1 - start_frac.y : start_frac.y,
                                                       arrow_sign.z > 0 ? 1 - start_frac.z : start_frac.z};

    while (current.x <= 1.0f || current.y <= 1.0f || current.z <= 1.0f) {
        // 找出current的xyz中最小的那个，步进该方向
        Direction dir;
        if (current.x < current.y) {
            if (current.x < current.z) {
                current_pos.x += arrow_sign.x;
                current.x += step.x;
                dir = arrow_sign.x > 0 ? Direction::WEST : Direction::EAST;
            } else {
                current_pos.z += arrow_sign.z;
                current.z += step.z;
                dir = arrow_sign.z > 0 ? Direction::NORTH : Direction::SOUTH;
            }
        } else {
            if (current.y < current.z) {
                current_pos.y += arrow_sign.y;
                current.y += step.y;
                dir = arrow_sign.y > 0 ? Direction::DOWN : Direction::UP;
            } else {
                current_pos.z += arrow_sign.z;
                current.z += step.z;
                dir = arrow_sign.z > 0 ? Direction::NORTH : Direction::SOUTH;
            }
        }

        if (BlockState *s = get_block_state(current_pos); s != nullptr) {
            if (s->get_block() != Blocks::get().AIR) {
                return {current_pos, dir, s};
            }
        }
    }

    return {};
}

} // namespace Craft