#pragma once

#include "UberShader.h"
#include "core/RefCount.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"

#include "resource/Resource.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {

struct ImageBindingInfo {
    Ref<GLTexture> image; // 为空表示未绑定
    uint32_t mipmap_level = 0;
    bool layered = false;
    uint32_t layer = 0;
    uint32_t gl_access = 0;
    uint32_t gl_type = 0;
};

class ShaderParameters : public Resource {
protected:
    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, MaterialParameter> parameters;
    // 纹理单元和图像单元由UberShader按声明顺序连续分配，两个容器在构造时就按单元数定容，单元可以直接作为下标
    std::vector<Ref<GLTexture>> textures;              // 单元 -> 纹理，为空表示使用着色器的默认纹理
    std::vector<ImageBindingInfo> images;              // 单元 -> 绑定，image为空表示未绑定
    std::vector<Ref<GLBuffer>> uniform_buffers;        // slot -> buffer
    std::vector<Ref<GLBuffer>> shader_storage_buffers; // slot -> buffer

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
        // 传空表示使用着色器的默认纹理，this->textures中允许出现空元素
        if (auto iter = uber_shader->get_texture_units().find(name); iter != uber_shader->get_texture_units().end()) {
            this->textures[iter->second.unit] = texture;
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
    void set_image(const std::string &name, const Ref<GLTexture> &texture, uint32_t mipmap_level = 0,
                   bool layered = false, uint32_t layer = 0);

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
