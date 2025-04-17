#include "Material.h"
#include "platform/graphics/Graphics.h"

namespace Goonya::Graphics {

void Material::bind() {
    this->update();

    graphics_api->set_pipeline_state(pipeline_state);
    shader->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    per_material->bind_uniform(uber_shader->per_material_block().binding);
    // 绑定所有纹理
    for (const auto &[unit, t] : textures) {
        t->bind(unit);
    }
}
void Material::set_param(const std::string &name, const Meta::DynamicData &value) {
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
    : uber_shader(uber_shader), local_variant_code(0),
      current_variant_code({uber_shader->get_global_key_code(), local_variant_code}) {

    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 创建此材质的ConstantBuffer
    per_material = graphics_api->create_buffer(uber_shader->per_material_block().layout.size, BufferType::DYNAMIC);
    is_parameters_dirty = true;
};

void Material::update_shader_variant() {
    VariantCodeSet code{.global_code = uber_shader->get_global_key_code(), .local_code = local_variant_code};
    if (current_variant_code != code) {
        shader = uber_shader->query_variant(code);
        current_variant_code = code;
    }
}

void Material::update_parameter() {
    if (!is_parameters_dirty)
        return;

    {
        const auto &layout = uber_shader->per_frame_block().layout;
        // 所有的参数都写一遍
        auto w = DynamicBufferWriter(per_material, layout, BufferMapOption::WRITE_DISCARD);
        for (const auto &[name, value] : parameters) {
            w.set_field(name, value);
        }
    }

    is_parameters_dirty = false;
}

} // namespace Goonya::Graphics
