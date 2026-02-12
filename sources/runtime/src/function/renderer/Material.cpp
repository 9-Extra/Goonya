#include "Material.h"
#include "core/RefCount.h"

#include "UberShader.h"
#include "function/renderer/PipelineLayout.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/OpenGLAPI.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <cstddef>
#include <cstring>
#include <utility>
#include <variant>

namespace Goonya {

void Material::bind() const {
    GL.set_pipeline_state(pipeline_setting);
    get_shader()->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    if (per_material->get_size() != 0) {
        get_per_material_uniform()->bind_uniform(PER_MATERIAL_UNIFORM_BINDING);
    }
    bind_external_resources();
}

void Material::bind_external_resources() const {
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

void Material::set_param(const std::string &name, const MaterialParameter &value) {
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

Material::Material(UberShader *uber_shader)
    : uber_shader(uber_shader), local_variant_code(0), pipeline_setting(uber_shader->get_pipeline_setting()),
      current_variant_code(uber_shader->get_effective_global_key_code(), local_variant_code) {

    shader = uber_shader->query_variant(current_variant_code); // 保证shader总不是空的

    // 创建此材质的ConstantBuffer
    per_material = create_ref<GLBuffer>(BufferType::MODIFIABLE, uber_shader->per_material_block().total_size);
    is_parameters_dirty = true;
};

Ref<Material> Material::clone() const noexcept {
    Ref<Material> c = create_ref<Material>(uber_shader);
    c->local_variant_code = local_variant_code;
    c->pipeline_setting = pipeline_setting;
    c->render_priority = render_priority;

    c->parameters = parameters;
    c->textures = textures;
    c->external_buffer = external_buffer;

    return c;
}

void Material::update_shader_variant() const {
    VariantCodeSet code{uber_shader->get_effective_global_key_code(), local_variant_code};
    if (current_variant_code != code) [[unlikely]] {
        shader = uber_shader->query_variant(code);
        current_variant_code = code;
    }
}

void Material::update_parameter() const {
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
            std::visit(
                [=](auto &&arg) {
                    if constexpr (!std::is_same_v<decltype(arg), std::monostate>) {
                        memcpy(base_ptr + field_info.offset, &arg, sizeof(arg));
                    } else {
                        std::unreachable();
                    }
                },
                param);
        }
        per_material->unmap();
    }
}

} // namespace Goonya
