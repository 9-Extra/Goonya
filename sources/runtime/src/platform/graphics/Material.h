#pragma once

#include "core/RefCount.h"
#include "core/assets.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/UberShader.h"
#include "resource/Resource.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Goonya::Graphics {

/**
 * @brief PipelineSetting中的条目保存在Material中的parameters中的类型
 */
using PipelineSettingParamType = int32_t;

class PipelineSettingSetter {
    struct Entry {
        std::string_view name;
        void (*setter)(PipelineSetting &, PipelineSettingParamType);
        PipelineSettingParamType (*getter)(const PipelineSetting &);
    };

    static constexpr Entry setter_table[] = {
        {"_depth_test",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(DepthTestMode::DISABLE));
             s.depth_test = DepthTestMode(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.depth_test); }},
        {"_cull_mode",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(CullFaceMode::DISABLE));
             s.cull_mode = CullFaceMode(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.cull_mode); }},
        {"_is_blend_enable",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= 1);
             s.is_blend_enable = bool(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.is_blend_enable); }},
        {"_blendop_color",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendOp::MAX));
             s.blendop_color = BlendOp(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.blendop_color); }},
        {"_blendop_alpha",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendOp::MAX));
             s.blendop_alpha = BlendOp(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.blendop_alpha); }},
        {"_src_color_factor",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendFactor::ONE_MINUS_DST_ALPHA));
             s.src_color_factor = BlendFactor(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.src_color_factor); }},
        {"_dst_color_factor",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendFactor::ONE_MINUS_DST_ALPHA));
             s.dst_color_factor = BlendFactor(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.dst_color_factor); }},
        {"_src_alpha_factor",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendFactor::ONE_MINUS_DST_ALPHA));
             s.src_alpha_factor = BlendFactor(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.src_alpha_factor); }},
        {"_dst_alpha_factor",
         [](PipelineSetting &s, PipelineSettingParamType v) {
             assert(v <= std::to_underlying(BlendFactor::ONE_MINUS_DST_ALPHA));
             s.dst_alpha_factor = BlendFactor(v);
         },
         [](const PipelineSetting &s) { return PipelineSettingParamType(s.dst_alpha_factor); }}};

public:
    PipelineSettingSetter() = delete;

    static void set_pipeline_setting(std::string_view param, uint8_t v, PipelineSetting &setting) {
        for (const auto &entry : setter_table) {
            if (entry.name == param) {
                entry.setter(setting, v);
            }
        }
    }

    static void replace_pipeline_setting(std::string_view param, const PipelineSetting &reference,
                                             PipelineSetting &setting) {
        for (const auto &entry : setter_table) {
            if (entry.name == param) {
                entry.setter(setting, entry.getter(reference));
                return;
            }
        }
    }

    static bool is_pipeline_setting(std::string_view param) {
        return std::ranges::any_of(setter_table, [param](auto &&t) { return t.name == param; });
    }
};

struct MaterialDesc {
    std::unordered_map<std::string, PipelineSettingParamType> override_pipeline_setting;
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::vector<std::tuple<std::string, TextureType, AssetKey>> textures; // (着色器中名称, 纹理类型，资源键)

    AssetKey uber_shader_name;
    std::vector<std::string> local_variant_keys;
};

class Material final : public RefCount {
protected:
    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, PipelineSettingParamType> override_pipeline_setting;
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::unordered_map<uint32_t, Ref<Texture>> textures; // slot -> texture
    std::unordered_map<uint32_t, std::tuple<Ref<Buffer>, BufferBindingType>>
        external_buffer; // slot -> (buffer, bindingtype)

    // 脏标记
    mutable bool is_parameters_dirty;

    // 设备侧
    PipelineSetting pipeline_setting;
    VariantCodeSet current_variant_code; // 当前绑定的着色器的变体码
    Ref<Shader> shader;
    Ref<Buffer> per_material; // 显存中的材质参数

public:
    // Material在创建是就对应特定的UberShader，且之后不能更改
    explicit Material(UberShader *uber_shader);
    ~Material() = default;

    void bind();
    void update() {
        update_shader_variant();
        update_parameter();
    }

    UberShader *get_uber_shader() const noexcept { return uber_shader; }

    void set_pipeline_setting(const std::string &name, PipelineSettingParamType value);
    void set_param(const std::string &name, const Meta::DynamicData &value);
    void set_external_buffer(const std::string &name, const Ref<Buffer> &buffer) {
        auto &info = uber_shader->get_uniform_info().at(name);
        external_buffer[info.binding] = {buffer, info.binding_type};
    }

    void set_texture(const std::string &name, const Ref<Texture> &texture) {
        // texture slot可能被优化掉了
        if (auto iter = uber_shader->get_texture_units().find(name); iter != uber_shader->get_texture_units().end()) {
            this->textures[iter->second] = texture;
        }
    }

    void set_local_variant_key(const std::string &key) {
        local_variant_code = uber_shader->set_local_variant_key(local_variant_code, key);
    }
    void remove_local_variant_key(const std::string &key) {
        local_variant_code = uber_shader->reset_local_variant_key(local_variant_code, key);
    }
    // 复制材质
    Ref<Material> clone() const noexcept;

protected:
    void update_shader_variant();
    void update_parameter();

private:
};

class MaterialContainer final : public Resource::ResourceContainer<MaterialContainer, MaterialDesc, Material> {
public:
    MaterialContainer() : ResourceContainer<MaterialContainer, MaterialDesc, Material>("材质") {}

    Ref<Material> load(const MaterialDesc &desc) const;
};

} // namespace Goonya::Graphics
