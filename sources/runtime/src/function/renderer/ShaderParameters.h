#pragma once

#include "UberShader.h"
#include "core/RefCount.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"

#include "resource/Resource.h"
#include "runtime/GAssert.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {

struct ImageBindingInfo {
    Ref<GLTexture> image;
    uint32_t mipmap_level;
    uint32_t layer;
    uint32_t gl_access;
    uint32_t gl_type;
};

class ShaderParameters : public Resource {
protected:
    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, MaterialParameter> parameters;
    std::unordered_map<uint32_t, Ref<GLTexture>> textures; // slot -> textures
    std::unordered_map<uint32_t, ImageBindingInfo> images; // slot -> textures
    std::vector<Ref<GLBuffer>> uniform_buffers;            // slot -> buffer
    std::vector<Ref<GLBuffer>> shader_storage_buffers;     // slot -> buffer

    // 脏标记
    mutable bool is_parameters_dirty;

    // 设备侧
    mutable VariantCodeSet current_variant_code; // 当前绑定的着色器的变体码
    mutable Ref<GLShader> shader;
    Ref<GLBuffer> per_material; // 显存中的材质参数

    // Material在创建是就对应特定的UberShader，且之后不能更改
    // 使用Material而不是这个
    explicit ShaderParameters(UberShader *uber_shader);

public:
    ~ShaderParameters() = default;

    // 使用Material而不是这个
    // void bind() const;

    /**
     * @brief 绑定额外的资源到渲染管线
     * 只包括外部缓冲区和纹理
     */
    void bind_external_resources() const;
    Ref<GLShader> get_shader() const noexcept {
        update_shader_variant();
        return shader;
    }

    Ref<GLBuffer> get_per_material_uniform() const noexcept {
        update_parameter();
        return per_material;
    }

    UberShader *get_uber_shader() const noexcept { return uber_shader; }

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

    void set_external_buffer(const std::string &name, const Ref<GLBuffer> &buffer);

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

    /**
     * @brief 设置材质的直接图像访问
     *
     * @param name 纹理名称
     * @param texture 纹理对象
     * @note 必须在bind()之前调用
     */
    void set_image(const std::string &name, const Ref<GLTexture> &texture, uint32_t mipmap_level, uint32_t layer) {
        // texture slot可能被优化掉了
        // 允许传入texture为空，表示使用默认纹理。但this->textures不应包含空指针
        if (auto iter = uber_shader->get_image_units().find(name); iter != uber_shader->get_image_units().end()) {
            uint32_t unit = iter->second.unit; // 在bind时再进行texture类型检查
            GN_ASSERT(iter->second.gl_access != 0 && iter->second.gl_type != 0);
            if (texture) {
                this->images[unit] = {
                    .image = texture,
                    .mipmap_level = mipmap_level,
                    .layer = layer,
                    .gl_access = iter->second.gl_access,
                    .gl_type = iter->second.gl_type,
                };
            } else {
                this->images.erase(unit);
            }
        } else {
            throw RuntimeError(std::format("图形\"{}\"不存在", name));
        }
    }

    bool set_local_variant_key(const std::string &key) {
        return uber_shader->set_local_variant_key(local_variant_code, key);
    }
    bool remove_local_variant_key(const std::string &key) {
        return uber_shader->reset_local_variant_key(local_variant_code, key);
    }
    // 复制材质
    Ref<ShaderParameters> clone() const noexcept;

protected:
    void update_shader_variant() const;
    void update_parameter() const;

private:
};

} // namespace Goonya
