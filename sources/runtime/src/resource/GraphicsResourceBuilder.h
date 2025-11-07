#pragma once
#include "core/metatype/metatype.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Texture.h"
#include <iterator>
#include <unordered_set>
#include <vector>

namespace Goonya::Resource {

class MaterialBuilder {
public:
    explicit MaterialBuilder(const AssetKey &uber_shader_name) { desc.uber_shader_name = uber_shader_name; }

    void set_variant_key(const std::string &key) noexcept { variant_keys.emplace(key); }
    MaterialBuilder &set_pipeline_setting(const std::string &name, Graphics::PipelineSettingParamType value) {
        desc.override_pipeline_setting.emplace(name, value);
        return *this;
    }

    template <Meta::meta_type T>
    MaterialBuilder &add_parameter(const std::string &name, const T &value) noexcept {
        desc.parameters.emplace(name, value);
        return *this;
    }

    MaterialBuilder &add_sampler(const std::string &name, Graphics::TextureType type,
                                 const std::string &texture_key) noexcept {
        desc.textures.emplace_back(name, type, texture_key);
        return *this;
    }

    Graphics::MaterialDesc build() {
        desc.local_variant_keys = std::vector<std::string>{std::make_move_iterator(variant_keys.begin()),
                                                           std::make_move_iterator(variant_keys.end())};
        return desc;
    }

private:
    std::unordered_set<std::string> variant_keys; // 去重
    Graphics::MaterialDesc desc;
};

} // namespace Goonya::Resource
