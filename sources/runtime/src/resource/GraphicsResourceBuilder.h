#pragma once
#include "core/metatype/metatype.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Texture.h"
#include <iterator>
#include <unordered_set>
#include <vector>

namespace Goonya {
namespace Resource {

class MaterialBuilder {
public:
    MaterialBuilder(const AssetKey &uber_shader_name) { desc.uber_shader_name = uber_shader_name; }

    void set_variant_key(const std::string &key) noexcept { varient_keys.emplace(key); }
    void set_depth_test_mode(Graphics::DepthTestMode mode) noexcept { desc.depth_test = mode; }
    void set_cull_mode(Graphics::CullFaceMode mode) noexcept { desc.cull_mode = mode; }

    template <Meta::meta_type T>
    MaterialBuilder &add_parameter(const std::string &name, const T &vaule) noexcept {
        desc.parameters.emplace(name, vaule);
        return *this;
    }

    MaterialBuilder &add_sampler(const std::string &name, Graphics::TextureType type, std::string texture_key) noexcept {
        desc.textures.emplace_back(name, type, texture_key);
        return *this;
    }

    Graphics::MaterialDesc build() {
        desc.variant_keys = std::vector<std::string>{std::make_move_iterator(varient_keys.begin()),
                                                     std::make_move_iterator(varient_keys.end())};
        return desc;
    }

private:
    std::unordered_set<std::string> varient_keys; // 去重
    Graphics::MaterialDesc desc;
};

} // namespace Resource
} // namespace Goonya