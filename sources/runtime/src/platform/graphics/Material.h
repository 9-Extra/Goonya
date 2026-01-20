#pragma once

#include "core/RefCount.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/UberShader.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/Resource.h"

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
    std::unordered_map<std::string, MaterialParameter> parameters;
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
    explicit Material(Ref<UberShader> uber_shader) : Material(uber_shader.get()) {}
    ~Material() = default;

    void bind();
    void update() {
        update_shader_variant();
        update_parameter();
    }

    UberShader *get_uber_shader() const noexcept { return uber_shader; }

    void set_pipeline_setting(const std::string &name, PipelineSettingParamType value);

    void set_param(const std::string &name, const MaterialParameter &value);
    MaterialParameter get_param(const std::string &name) const noexcept {
        if (parameters.contains(name)) {
            return parameters.at(name);
        } else {
            const auto &uber_shader_fields = uber_shader->per_material_block().fields;
            auto iter = uber_shader_fields.find(name);
            return iter != uber_shader_fields.end() ? iter->second.type_and_default_value : MaterialParameter();
        }
    }

    void set_external_buffer(const std::string &name, const Ref<GLBuffer> &buffer) {
        GN_ASSERT(buffer);
        auto info = uber_shader->get_uniform_info(name);
        if (info.has_value()) {
            auto [binding, type] = info.value();
            external_buffer[binding] = {buffer, type};
        } else {
            // Maybe optimized out?
        }
    }

    /**
     * @brief 设置材质的纹理
     *
     * @param name 纹理名称
     * @param texture 纹理对象
     * @note 必须在bind()之前调用
     */
    void set_texture(const std::string &name, const Ref<GLTexture> &texture) {
        // texture slot可能被优化掉了
        // 允许传入texture为空，表示使用默认纹理。但this->textures不应包含空指针
        if (auto iter = uber_shader->get_texture_units().find(name); iter != uber_shader->get_texture_units().end()) {
            uint32_t unit = iter->second.unit; // 在bind时再进行texture类型检查
            if (texture) {
                this->textures[unit] = texture;
            } else {
                this->textures.erase(unit);
            }
        } else {
            // 允许设置不存在的纹理，会被忽略
        }
    }

    bool set_local_variant_key(const std::string &key) {
        return uber_shader->set_local_variant_key(local_variant_code, key);
    }
    bool remove_local_variant_key(const std::string &key) {
        return uber_shader->reset_local_variant_key(local_variant_code, key);
    }
    // 复制材质
    Ref<Material> clone() const noexcept;

protected:
    void update_shader_variant();
    void update_parameter();

private:
};

} // namespace Goonya
