#pragma once

#include "core/assets.h"
#include "core/RefCount.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/UberShader.h"
#include "resource/Resource.h"
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Goonya::Graphics {

enum class CullFaceMode {
    BACK = 0,
    FRONT,
    FRONT_AND_BACK,
    DISABLE,
};

enum class DepthTestMode { LESS = 0, LESS_EQUAL, GREATER, GREATER_EQUAL, NEVER, ALWAYS, DISABLE };

struct PipeLineState {
    DepthTestMode depth_test = DepthTestMode::LESS;
    CullFaceMode cull_mode = CullFaceMode::BACK;
};

struct MaterialDesc {
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::vector<std::tuple<std::string, TextureType, AssetKey>> textures; // (着色器中名称, 纹理类型，资源键)

    AssetKey uber_shader_name;
    std::vector<std::string> local_variant_keys;

    PipeLineState pipeline_state;
};

class Material final : public RefCount {
protected:
    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;
    // 渲染管线设置
    PipeLineState pipeline_state;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::unordered_map<uint32_t, Ref<Texture>> textures; // slot -> texture
    std::unordered_map<uint32_t, std::tuple<Ref<Buffer>, BufferBindingType>> external_buffer; // slot -> (buffer, bindingtype) 

    // 脏标记
    mutable bool is_parameters_dirty;

    // 设备侧
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

    void set_pipeline_state(const PipeLineState &state) noexcept { pipeline_state = state; }

    void set_param(const std::string &name, const Meta::DynamicData &value);
    void set_external_buffer(const std::string &name, const Ref<Buffer> &buffer){
        auto& info = uber_shader->get_uniform_info().at(name);
        external_buffer[info.binding] = {buffer, info.binding_type};
    }

    void set_texture(const std::string &name, const Ref<Texture> &texture) {
        // texture slot可能被优化掉了
        if (auto iter = uber_shader->get_texture_units().find(name);iter != uber_shader->get_texture_units().end()){
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
};

class MaterialContainer final : public Resource::ResourceContainer<MaterialDesc, Material> {
public:
    MaterialContainer() : ResourceContainer<MaterialDesc, Material>("材质") {}

protected:
    Ref<Material> load(const MaterialDesc &desc) const override;
};

} // namespace Goonya::Graphics
