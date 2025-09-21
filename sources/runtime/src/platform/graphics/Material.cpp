#include "Material.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Shader.h"
#include "resource/ResMng.h"
#include <cassert>
#include <utility>

namespace Goonya::Graphics {

void Material::bind() {
    this->update();

    graphics_api->set_pipeline_state(pipeline_state);
    shader->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    if (per_material->get_size() != 0){
        per_material->bind_uniform(uber_shader->per_material_block().binding);
    }
    // 绑定所有纹理
    for (const auto &[unit, t] : textures) {
        t->bind(unit);
    }
    // 绑定其他buffer
    for (const auto &[binding, buffer_with_type] : external_buffer) {
        const auto& [buffer, type] = buffer_with_type;
        if (type == BufferBindingType::UNIFORM){
            buffer->bind_uniform(binding);
        } else if (type == BufferBindingType::SHADER_STORAGE){
            buffer->bind_storage(binding);
        } else {
            std::unreachable();
        }
    }
}
void Material::set_param(const std::string &name, const Meta::DynamicData &value) {
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
    : uber_shader(uber_shader), local_variant_code(0),
      current_variant_code(uber_shader->get_global_key_code(), local_variant_code) {

    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 创建此材质的ConstantBuffer
    per_material = graphics_api->create_buffer(uber_shader->per_material_block().layout.size, BufferType::DYNAMIC);
    is_parameters_dirty = true;
};

intrusive_ptr<Material> Material::clone() const noexcept{
    intrusive_ptr<Material> c = make_intrusive<Material>(uber_shader);
    c->local_variant_code = local_variant_code;
    c->pipeline_state = pipeline_state;

    c->parameters = parameters;
    c->textures = textures;
    c->external_buffer = external_buffer;

    return c;
}

void Material::update_shader_variant() {
    VariantCodeSet code{uber_shader->get_global_key_code(), local_variant_code};
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

intrusive_ptr<Material> MaterialContainer::load(const MaterialDesc &desc) const {
    intrusive_ptr<Graphics::Material> mat =
        make_intrusive<Graphics::Material>(resources.shader_lib->query_uber_shader(desc.uber_shader_name));
    mat->set_pipeline_state(desc.pipeline_state);

    for (const auto &[name, value] : desc.parameters) {
        mat->set_param(name, value);
    }
    for (const auto &[name, texture_type, texture_key] : desc.textures) {
        switch (texture_type) {

        case Graphics::TextureType::UNKNOWN: {
            throw RuntimeError(std::format("纹理资源\"{}\"类型未指定", texture_key));
        }
        case Graphics::TextureType::TEXTURE_2D: {
            mat->set_texture(name, resources.texture2ds.get(texture_key));
            break;
        }
        case Graphics::TextureType::TEXTURE_CUBEMAP: {
            mat->set_texture(name, resources.cubemaps.get(texture_key));
            break;
        }
        default: {
            assert(false); // todo
        }
        }
    }
    return mat;
};
} // namespace Goonya::Graphics
