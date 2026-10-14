#include "ShaderParameters.h"
#include "core/RefCount.h"

#include "UberShader.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <rfl/enums.hpp>
#include <utility>
#include <variant>

namespace Goonya {

void ShaderParameters::bind_external_resources() const {
    // 绑定所有纹理
    for (const auto &[name, info] : uber_shader->get_texture_units()) {
        const Ref<GLTexture> &texture = textures[info.unit];
        if (texture) {
            texture->bind_texture(info.unit); // 绑定材质设定的纹理
        } else {
            // 绑定默认纹理
            uber_shader->get_default_texture(info.unit)->bind_texture(info.unit);
        }
    }
    for (const auto &[name, info] : uber_shader->get_image_units()) {
        const ImageBindingInfo &image = images[info.unit];
        if (!image.image) {
            // 图像未绑定是不允许的，它没有默认
            throw RuntimeError(std::format("着色器\"{}\"中图像\"{}\"未绑定", uber_shader->get_name(), name));
        }
        // 图像类型在set_image时已经检查过，这里兜底
        GN_ASSERT_MSG(info.type == TextureType::UNKNOWN || image.image->get_type() == info.type,
                      "图像\"{}\"在着色器中为{}类型，实际绑定了{}类型", name, rfl::enum_to_string(info.type),
                      rfl::enum_to_string(image.image->get_type()));
        image.image->bind_image(info.unit, image.mipmap_level, image.layered, image.layer, image.gl_access,
                                image.gl_type);
    }
    // 绑定其他buffer
    for (const auto &[binding, buffer] : std::views::enumerate(uniform_buffers)) {
        if (buffer) {
            buffer->bind_uniform((uint32_t)binding);
        }
    }
    for (const auto &[binding, buffer] : std::views::enumerate(shader_storage_buffers)) {
        if (buffer) {
            buffer->bind_storage((uint32_t)binding);
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

void ShaderParameters::set_image(const std::string &name, const Ref<GLTexture> &texture, uint32_t mipmap_level,
                                 bool layered, uint32_t layer) {
    // texture slot可能被优化掉了
    // 传空表示不绑定图像，bind时会因为图像未绑定而报错
    auto iter = uber_shader->get_image_units().find(name);
    if (iter == uber_shader->get_image_units().end()) {
        throw RuntimeError(std::format("图形\"{}\"不存在", name));
    }
    const uint32_t unit = iter->second.unit;
    if (!texture) {
        this->images[unit] = {};
        return;
    }

    GN_ASSERT(iter->second.gl_access != 0 && iter->second.gl_type != 0);
    // 着色器中图像的类型只能靠反射得到，未被任何已编译变体使用的图像会停留在UNKNOWN，此时无法检查
    if (iter->second.type != TextureType::UNKNOWN && texture->get_type() != iter->second.type) {
        throw RuntimeError(std::format("图像\"{}\"在着色器中为{}类型，实际绑定了{}类型", name,
                                       rfl::enum_to_string(iter->second.type),
                                       rfl::enum_to_string(texture->get_type())));
    }
    if (mipmap_level >= texture->get_mipmap_levels()) {
        throw RuntimeError(std::format("图像\"{}\"的mipmap_level {}超过上限{}", name, mipmap_level,
                                       texture->get_mipmap_levels() - 1));
    }
    const auto layer_count = texture->get_layer_count();
    if (!layered && layer_count.has_value() && layer >= *layer_count) {
        throw RuntimeError(std::format("图像\"{}\"的layer {}超过上限{}", name, layer, *layer_count - 1));
    }

    this->images[unit] = {
        .image = texture,
        .mipmap_level = mipmap_level,
        .layered = layered,
        .layer = layer,
        .gl_access = iter->second.gl_access,
        .gl_type = iter->second.gl_type,
    };
}

ShaderParameters::ShaderParameters(UberShader *uber_shader)
    : uber_shader(uber_shader), local_variant_code(0),
      current_variant_code(uber_shader->get_effective_global_key_code(), local_variant_code) {
    if (!uber_shader) {
        throw RuntimeError("无效元着色器，可能元着色器加载时失败了");
    }
    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 纹理单元和图像单元由UberShader按声明顺序连续分配，构造时就能确定单元数，之后直接以单元为下标访问
    textures.resize(uber_shader->get_texture_units().size());
    images.resize(uber_shader->get_image_units().size());

    // 创建此材质的ConstantBuffer
    per_material = create_ref<GLBuffer>(BufferType::MODIFIABLE, uber_shader->per_material_block().total_size);
    is_parameters_dirty = true;
};

Ref<ShaderParameters> ShaderParameters::clone() const noexcept {
    Ref<ShaderParameters> c = Ref<ShaderParameters>{new ShaderParameters{uber_shader}};
    c->local_variant_code = local_variant_code;

    c->parameters = parameters;
    c->textures = textures;
    c->images = images;
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
