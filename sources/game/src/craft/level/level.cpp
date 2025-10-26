#include "level.h"

#include "core/cgmath.h"
#include "craft/block/all_blocks.h"
#include "craft/block/blockstate.h"
#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include "craft/level/RayCast.h"
#include "craft/level/chunk.h"
#include "function/world/World.h"
#include <cassert>

namespace Craft {

Level::Level(Goonya::World *world, const std::shared_ptr<Goonya::GObject> &player)
    : Goonya::TickFunction(Goonya::TickType::FIXED_TICK), bind_world(world), level_renderer(world->main_scene()),
      delta_time_residual(GameClock::duration::zero()) {
    assert(world != nullptr && player != nullptr);

    this->player = player;
    player_chunk_pos = {255, 255, 255}; // 保证第一帧 ChunkPos(BlockPos(player_pos)) != play_chunk_pos
}

void Level::tick() {
    load_chunks();

    Goonya::Vector3f player_pos = player->get_world_model_matrix().resolve_position();
    Goonya::Vector3f player_dir = Goonya::Vector3f{0, 0, -1} * player->get_world_model_matrix().to_matrix3();
    BlockHitResult hit_result = ray_cast(Ray{player_pos, player_dir}, 64);

    if (hit_result) {
        // LOG_INFO("{} {}", hit_result.normal, hit_result.block_state->get_block()->get_display_name());
        BlockPos pos = BlockPos{hit_result.position + get_direction_vector(hit_result.normal)};
        set_block_state(pos, Blocks::get().GRANITE->get_default_blockstate());
    } else {
        // LOG_INFO("No hit");
    }

    level_renderer.render_frame();
}

void Level::load_chunks() {
    // 玩家位置
    Goonya::Vector3f player_pos = player->get_world_model_matrix().resolve_position();

    ChunkPos current_player_chunk_pos = ChunkPos(BlockPos(player_pos));
    if (current_player_chunk_pos == player_chunk_pos) {
        return;
    } else {
        player_chunk_pos = current_player_chunk_pos;
        auto processed_chunk_receiver = [level = Ref{this}](const Ref<Chunk> &chunk) mutable {
            level->accessible_chunk.emplace(chunk->chunk_pos, chunk);
            level->level_renderer.register_chunk(chunk);
        };
        // 添加区块加载任务
        for (int32_t x = player_chunk_pos.x - chunk_load_distance; x <= player_chunk_pos.x + chunk_load_distance; x++) {
            for (int32_t y = player_chunk_pos.y - chunk_load_distance; y <= player_chunk_pos.y + chunk_load_distance;
                 y++) {
                for (int32_t z = player_chunk_pos.z - chunk_load_distance;
                     z <= player_chunk_pos.z + chunk_load_distance; z++) {
                    ChunkPos chunk_pos = ChunkPos{x, y, z};
                    if (!all_chunks.contains(chunk_pos)) {
                        Ref<Chunk> chunk = all_chunks.emplace(chunk_pos, create_ref<Chunk>(chunk_pos)).first->second;
                        chunk_generator.process_chunk_async(chunk, processed_chunk_receiver);
                    }
                }
            }
        }
    }

    // 收集加载完成的区块到visible_chunks
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