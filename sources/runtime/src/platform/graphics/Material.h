#pragma once

#include "core/RefCount.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/Resource.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/UberShader.h"
#include "resource/Resource.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

namespace Goonya {

class Material final : public Resource {
protected:
    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, PipelineSettingParamType> override_pipeline_setting;
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::unordered_map<uint32_t, Ref<GLTexture>> textures; // slot -> texture
    std::unordered_map<uint32_t, std::tuple<Ref<GLBuffer>, BufferBindingType>>
        external_buffer; // slot -> (buffer, bindingtype)

    // 脏标记
    mutable bool is_parameters_dirty;

    // 设备侧
    PipelineSetting pipeline_setting;
    VariantCodeSet current_variant_code; // 当前绑定的着色器的变体码
    Ref<GLShader> shader;
    Ref<GLBuffer> per_material; // 显存中的材质参数

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
    template<Meta::meta_type T>
    void set_param(const std::string &name, const T &value){
        set_param(name, Meta::DynamicData(value));
    }
    
    void set_external_buffer(const std::string &name, const Ref<GLBuffer> &buffer) {
        assert(buffer);
        auto &info = uber_shader->get_uniform_info().at(name);
        external_buffer[info.binding] = {buffer, info.binding_type};
    }

    void set_texture(const std::string &name, const Ref<GLTexture> &texture) {
        // texture slot可能被优化掉了
        assert(texture);
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

} // namespace Goonya
