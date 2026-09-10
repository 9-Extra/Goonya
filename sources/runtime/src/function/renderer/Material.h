#pragma once

#include "core/RefCount.h"
#include "function/renderer/ShaderParameters.h"
#include "platform/graphics/PipelineSetting.h"

namespace Goonya {

class Material final : public ShaderParameters {
protected:
    std::unordered_map<std::string, PipelineSettingParamType> override_pipeline_setting;
    PipelineSetting pipeline_setting;
    std::optional<RenderPriority> render_priority;

public:
    explicit Material(UberShader *uber_shader)
        : ShaderParameters(uber_shader), pipeline_setting(uber_shader->get_pipeline_setting()) {
        if (!uber_shader->is_graphics_shader()) {
            throw RuntimeError(std::format("\"{}\" 不是光栅化管线的元着色器", uber_shader->get_name()));
        }
    };
    explicit Material(Ref<UberShader> uber_shader) : Material(uber_shader.get()) {}

    PipelineSetting get_pipeline_setting() const noexcept { return pipeline_setting; }
    RenderPriority get_render_priority() const noexcept {
        return render_priority.has_value() ? render_priority.value() : uber_shader->get_render_priority();
    }

    Ref<Material> clone() const noexcept;

    /**
     * @brief 绑定材质到渲染管线
     * 完整的绑定，包含着色器、uniform buffer、纹理等
     */
    void bind() const;
    void set_pipeline_setting(const std::string &name, PipelineSettingParamType value);
};

} // namespace Goonya