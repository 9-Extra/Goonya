#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Goonya {
namespace Graphics {

struct MaterialDesc {
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::vector<std::tuple<std::string, std::string>> textures; // (着色器中名称, 资源键)

    std::string uber_shader_name;
    std::vector<std::string> variant_keys; // 不区分全局和局部

    DepthTestMode depth_test = DepthTestMode::LESS;
    CullFaceMode cull_mode = CullFaceMode::BACK;
};

class Material : public intrusive_ptr_base<Material> {
public:
    virtual void bind() = 0;
    virtual void update() = 0;

    virtual ~Material() = default;

    void set_depth_test_mode(DepthTestMode mode) noexcept { depth_test = mode; }
    void set_cull_mode(CullFaceMode mode) noexcept { cull_mode = mode; }

    void set_param(const std::string &name, const Meta::DynamicData &value) {
        if (auto iter = parameters.find(name); iter != parameters.end()) {
            if (iter->second != value) {
                iter->second = value;
                is_parameters_dirty = true;
            }
        } else {
            parameters.emplace(name, value);
            is_parameters_dirty = true;
        }
    }

    void set_texture(const std::string &name, intrusive_ptr<Texture> texture) {
        uint32_t slot = uber_shader->get_texture_units().at(name);
        this->textures[slot] = texture;
    }

    void set_loacl_variant_key(const std::string &key) { uber_shader->set_loacl_variant_key(local_variant_code, key); }
    void remove_loacl_variant_key(const std::string &key) {
        uber_shader->reset_loacl_variant_key(local_variant_code, key);
    }

protected:
    // Material在创建是就对应特定的UberShader，且之后不能更改
    Material(UberShader *uber_shader) : uber_shader(uber_shader) {
        is_parameters_dirty = true;

        depth_test = DepthTestMode::LESS;
        cull_mode = CullFaceMode::BACK;

        local_variant_code = 0;
        current_variant_code.global_code = uber_shader->get_global_key_code();
        current_variant_code.loacl_code = local_variant_code;
        shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的
    };

    // 着色器设置
    UberShader *const uber_shader;
    VariantCode local_variant_code;
    // 渲染管线设置
    DepthTestMode depth_test;
    CullFaceMode cull_mode;

    // 所有参数在内存中保存一份
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::unordered_map<uint32_t, intrusive_ptr<Texture>> textures; // slot -> texture

    // 脏标记
    mutable bool is_parameters_dirty;

    // 设备侧
    VariantCodeSet current_variant_code; // 当前绑定的着色器的变体码
    intrusive_ptr<Shader> shader;
    intrusive_ptr<Buffer> per_material; // 显存中的材质参数
};

} // namespace Graphics
} // namespace Goonya