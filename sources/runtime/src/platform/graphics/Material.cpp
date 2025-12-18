#include "Material.h"
#include "core/RefCount.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/UberShader.h"
#include "runtime/GoonyaException.h"

#include <cassert>
#include <utility>

namespace Goonya::Graphics {

void Material::bind() {
    this->update();

    graphics_api->set_pipeline_state(pipeline_setting);
    shader->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    if (per_material->get_size() != 0) {
        per_material->bind_uniform(uber_shader->per_material_block().binding);
    }
    // 绑定所有纹理
    for (const auto &[unit, t] : textures) {
        t->bind(unit);
    }
    // 绑定其他buffer
    for (const auto &[binding, buffer_with_type] : external_buffer) {
        const auto &[buffer, type] = buffer_with_type;
        if (type == BufferBindingType::UNIFORM) {
            buffer->bind_uniform(binding);
        } else if (type == BufferBindingType::SHADER_STORAGE) {
            buffer->bind_storage(binding);
        } else {
            std::unreachable();
        }
    }
}

void Material::set_pipeline_setting(const std::string &name, PipelineSettingParamType value) {
    if (!PipelineSettingSetter::is_pipeline_setting(name)) {
        throw RuntimeError(std::format("渲染管线状态{}不存在", name));
    }
    if (value >= 0) {
        // 更新pipeline_setting，不需要加脏标记
        PipelineSettingSetter::set_pipeline_setting(name, value, pipeline_setting);
        override_pipeline_setting.emplace(name, value);
    } else {
        // 重置为着色器默认值
        PipelineSettingSetter::replace_pipeline_setting(name, uber_shader->get_pipeline_setting(), pipeline_setting);
        override_pipeline_setting.erase(name);
    }
}

void Material::set_param(const std::string &name, const Meta::DynamicData &value) {
    // 一般的材质属性
    assert(uber_shader->per_material_block().layout.fields.contains(name));
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

Material::Material(UberShader *uber_shader)
    : uber_shader(uber_shader), local_variant_code(0), pipeline_setting(uber_shader->get_pipeline_setting()),
      current_variant_code(uber_shader->get_effective_global_key_code(), local_variant_code) {

    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 创建此材质的ConstantBuffer
    per_material = graphics_api->create_buffer(uber_shader->per_material_block().layout.size, BufferType::DYNAMIC);
    is_parameters_dirty = true;
};

Ref<Material> Material::clone() const noexcept {
    Ref<Material> c = create_ref<Material>(uber_shader);
    c->local_variant_code = local_variant_code;
    c->pipeline_setting = pipeline_setting;

    c->parameters = parameters;
    c->textures = textures;
    c->external_buffer = external_buffer;

    return c;
}

void Material::update_shader_variant() {
    VariantCodeSet code{uber_shader->get_effective_global_key_code(), local_variant_code};
    if (current_variant_code != code) {
        shader = uber_shader->query_variant(code);
        current_variant_code = code;
    }
}

void Material::update_parameter() {
    if (!is_parameters_dirty)
        return;

    {
        const auto &layout = uber_shader->per_material_block().layout;
        // 所有的参数都写一遍
        auto w = DynamicBufferWriter(per_material, layout, BufferMapOption::WRITE_DISCARD);
        for (const auto &[name, value] : parameters) {
            w.set_field(name, value);
        }
    }

    is_parameters_dirty = false;
}

} // namespace Goonya::Graphics
