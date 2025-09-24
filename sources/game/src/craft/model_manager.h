#pragma once

#include "TextureArrayAllocator.h"
#include "block/block_model.h"
#include "block/blockstate.h"
#include "platform/graphics/Texture.h"

#include <cassert>
#include <optional>
#include <unordered_map>

namespace Craft {

class ModelManager {
private:
    static std::optional<ModelManager> instance;

    std::unordered_map<const BlockState *, BakedBlockModel> blockstate_model_map;
    BakedBlockModel missing_block_model;

    // 既然BakedBlockModel中包含了对texture_array下标的引用，那么把texture放在ModelManager里也不过分吧
    Ref<Goonya::Graphics::Texture> block_texture_array;

public:
    ModelManager() { load_all_models(); };

    const BakedBlockModel& get_baked_model(const BlockState* state){
        if (auto iter = blockstate_model_map.find(state);iter != blockstate_model_map.end()){
            return iter->second;
        } else {
            return missing_block_model;
        }
    }

    Ref<Goonya::Graphics::Texture> get_textures() const noexcept {
        return block_texture_array;
    }

    static ModelManager &get() noexcept {
        assert(instance.has_value());
        return instance.value();
    }

    static void initalize();

private:
    void load_all_models();
    static BakedBlockModel bake_model(const BlockModel &model_src, TextureArrayAllocator &texture_allocator);
};

} // namespace Craft