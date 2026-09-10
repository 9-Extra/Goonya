#include "Material.h"

#include "function/renderer/PipelineLayout.h"
#include "platform/graphics/Graphics.h"

namespace Goonya {

Ref<Material> Material::clone() const noexcept {
    Ref<Material> c = create_ref<Material>(uber_shader);
    c->local_variant_code = local_variant_code;
    c->parameters = parameters;
    c->textures = textures;
    c->uniform_buffers = uniform_buffers;
    c->shader_storage_buffers = shader_storage_buffers;

    c->pipeline_setting = pipeline_setting;
    c->render_priority = render_priority;
    return c;
}

void Material::bind() const {
    GL.set_pipeline_state(pipeline_setting);
    get_shader()->bind_draw(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    if (per_material->get_size() != 0) {
        get_per_material_uniform()->bind_uniform(PER_MATERIAL_UNIFORM_BINDING);
    }
    bind_external_resources();
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

} // namespace Goonya