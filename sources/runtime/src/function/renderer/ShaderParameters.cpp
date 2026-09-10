#include "ShaderParameters.h"
#include "core/RefCount.h"

#include "UberShader.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "runtime/GAssert.h"

#include <cstddef>
#include <cstring>
#include <ranges>
#include <utility>
#include <variant>

namespace Goonya {

void ShaderParameters::bind_external_resources() const {
    // 绑定所有纹理
    for (const auto &[name, info] : uber_shader->get_texture_units()) {
        if (auto iter = textures.find(info.unit); iter != textures.end()) {
            GN_ASSERT(iter->second);
            /*
            if (iter->second->get_type() != info.type) {
                LOG_ERROR("材质绑定的纹理{}类型与着色器要求的类型不一致", name);
                iter->second = uber_shader->get_default_texture(info.unit); // 重置为默认纹理
            }
            */
            iter->second->bind(info.unit); // 绑定材质设定的纹理
        } else {
            // 绑定默认纹理
            uber_shader->get_default_texture(info.unit)->bind(info.unit);
        }
    }
    // 绑定其他buffer
    for (const auto &[binding, buffer] : std::views::enumerate(uniform_buffers)) {
        if (buffer) {
            buffer->bind_uniform(binding);
        }
    }
    for (const auto &[binding, buffer] : std::views::enumerate(shader_storage_buffers)) {
        if (buffer) {
            buffer->bind_storage(binding);
        }
    }
}

void ShaderParameters::set_param(const std::string &name, const MaterialParameter &value) {
    // 一般的材质属性
    GN_ASSERT_MSG(uber_shader->per_material_block().fields.contains(name), "材质参数{}不存在", name);
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

void ShaderParameters::set_external_buffer(const std::string &name, const Ref<GLBuffer> &buffer) {
    auto info = uber_shader->get_uniform_info(name);
    if (info.has_value()) {
        auto [binding, type] = info.value();
        switch (type) {
        case BufferBindingType::UNIFORM: {
            if (uniform_buffers.size() <= binding) {
                uniform_buffers.resize(binding + 1);
            }
            uniform_buffers[binding] = buffer;
            break;
        }
        case BufferBindingType::SHADER_STORAGE:
            if (shader_storage_buffers.size() <= binding) {
                shader_storage_buffers.resize(binding + 1);
            }
            shader_storage_buffers[binding] = buffer;
            break;
        }
    } else {
        LOG_WARN("缓冲区\"{}\"不存在于元着色器\"{}\"，可能是被优化了？", name, uber_shader->get_name());
    }
}

ShaderParameters::ShaderParameters(UberShader *uber_shader)
    : uber_shader(uber_shader), local_variant_code(0),
      current_variant_code(uber_shader->get_effective_global_key_code(), local_variant_code) {

    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 创建此材质的ConstantBuffer
    per_material = create_ref<GLBuffer>(BufferType::MODIFIABLE, uber_shader->per_material_block().total_size);
    is_parameters_dirty = true;
};

Ref<ShaderParameters> ShaderParameters::clone() const noexcept {
    Ref<ShaderParameters> c = Ref<ShaderParameters>{new ShaderParameters{uber_shader}};
    c->local_variant_code = local_variant_code;

    c->parameters = parameters;
    c->textures = textures;
    c->uniform_buffers = uniform_buffers;
    c->shader_storage_buffers = shader_storage_buffers;

    return c;
}

void ShaderParameters::update_shader_variant() const {
    VariantCodeSet code{uber_shader->get_effective_global_key_code(), local_variant_code};
    if (current_variant_code != code) [[unlikely]] {
        shader = uber_shader->query_variant(code);
        current_variant_code = code;
    }
}

void ShaderParameters::update_parameter() const {
    if (!is_parameters_dirty) return;
    is_parameters_dirty = false;

    {
        const auto &fields = uber_shader->per_material_block().fields;
        // 所有的参数都写一遍
        std::byte *base_ptr = per_material->map(BufferMapOption::WRITE_DISCARD);
        for (const auto &[name, field_info] : fields) {
            const MaterialParameter &param =
                parameters.contains(name) ? parameters.at(name) : field_info.type_and_default_value;
            GN_ASSERT(param.index() == field_info.type_and_default_value.index()); // 保证类型一致
            std::visit([=](auto &&arg) { memcpy(base_ptr + field_info.offset, &arg, sizeof(arg)); }, param);
        }
        per_material->unmap();
    }
}

} // namespace Goonya
