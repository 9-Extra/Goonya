#pragma once

#include "TextureArrayAllocator.h"
#include "block/block_model.h"
#include "block/blockstate.h"
#include "craft/core/core.h"
#include "craft/core/craft_math.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Craft {

class ModelSelector {
    std::vector<BakedBlockModel> models;
    std::vector<uint32_t> weights;
    uint32_t total_weight;
public:
    explicit ModelSelector(BakedBlockModel &&m) : models({std::move(m)}) {
        // 单个模型的情况
        weights = {1};
        total_weight = 1;
    }

    explicit ModelSelector(std::vector<BakedBlockModel> &&m, std::vector<uint32_t> &&w)
        : models(std::move(m)), weights(std::move(w)) {
        // 按权重分配
        assert(models.size() == weights.size());
        total_weight = std::ranges::fold_left(weights, 0, std::plus<>{});
    }

    const BakedBlockModel &select_model(uint32_t rand) const noexcept {
        if (models.size() == 1) [[likely]] {
            return models[0]; // 捷径，大多数模型不存在多选
        }

        rand %= total_weight;
        uint32_t sum = 0;
        for (size_t i = 0; i < models.size(); i++) {
            sum += weights[i];
            if (rand < sum) {
                return models[i];
            }
        }
        std::unreachable();
    }
};

class ModelManager {
private:
    static std::optional<ModelManager> instance;

    std::unordered_map<const BlockState *, ModelSelector> blockstate_model_map;
    BakedBlockModel missing_block_model;

    // 既然BakedBlockModel中包含了对texture_array下标的引用，那么把texture放在ModelManager里也不过分吧
    Ref<Goonya::GLTexture> block_texture_array;

public:
    ModelManager() { load_all_models(); };

    const BakedBlockModel &get_baked_model(const BlockState *state, BlockPos pos) {
        uint32_t seed = (uint32_t)splitmix64((uint64_t)get_seed(pos.x, pos.y, pos.z));
        return get_baked_model(state, seed);
    }

    const BakedBlockModel &get_baked_model(const BlockState *state, uint32_t rand) {
        if (auto iter = blockstate_model_map.find(state); iter != blockstate_model_map.end()) {
            return iter->second.select_model(rand);
        } else {
            return missing_block_model;
        }
    }

    Ref<Goonya::GLTexture> get_textures() const noexcept { return block_texture_array; }

    static ModelManager &get() noexcept {
        assert(instance.has_value());
        return instance.value();
    }

    static void initalize();

private:
    void load_all_models();
    static BakedBlockModel bake_model(const BlockModel &model_src, int32_t rotation_x, int32_t rotation_y, bool uvlock, TextureArrayAllocator &texture_allocator);
};

} // namespace Craft
