#include "MaterialLoader.h"

#include "core/RefCount.h"
#include "core/log/Log.h"
#include "function/renderer/Material.h"
#include "function/renderer/UberShader.h"
#include "resource/ResMng.h"
#include "resource/loader/MaterialParameterParser.h"

#include <format>

namespace Goonya {

Ref<Resource> MateriaLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                  const Json::Value &content) {
    const Json::Value &material_desc = content;

    if (!material_desc.isMember("uber_shader")) {
        throw RuntimeError("元着色器名称uber_shader字段缺失");
    }
    const std::string shader_name = material_desc["uber_shader"].asString();
    Ref<UberShader> shader = resources.load_resource<UberShader>(shader_name);
    if (!shader) {
        throw RuntimeError(std::format("元着色器{}加载失败", shader_name));
    }

    // 初始化材质
    Ref<Material> mat = create_ref<Material>(shader.get());

    // 变体
    for (const auto &variant_key : material_desc["variant_keys"]) {
        bool success = mat->set_local_variant_key(variant_key.asString());
        if (!success) {
            LOG_WARN("材质\"{}\"使用的着色器\"{}\"中不存在变体键\"{}\"", name, shader_name, variant_key.asString());
        }
    }

    // 材质覆盖的渲染管线设置
    const Json::Value &config = material_desc["config"];
    {
        PipelineSettingParamType value;
        const Json::Value &cull_mode = config["cull_mode"];
        if (!cull_mode) {
            goto NEXT;
        } else if (cull_mode == "back") {
            value = PipelineSettingParamType(CullFaceMode::BACK);
        } else if (cull_mode == "front") {
            value = PipelineSettingParamType(CullFaceMode::FRONT);
        } else if (cull_mode == "front_back") {
            value = PipelineSettingParamType(CullFaceMode::FRONT_AND_BACK);
        } else {
            throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", cull_mode.asString()));
        }
        mat->set_pipeline_setting("_cull_mode", value);
    }
NEXT: {
    PipelineSettingParamType value;
    const Json::Value &depth_test_mode = config["depth_func"];
    if (!depth_test_mode) {
        goto NEXT2;
    } else if (depth_test_mode == "less") {
        value = PipelineSettingParamType(DepthTestMode::LESS);
    } else if (depth_test_mode == "never") {
        value = PipelineSettingParamType(DepthTestMode::NEVER);
    } else if (depth_test_mode == "less_equal") {
        value = PipelineSettingParamType(DepthTestMode::LESS_EQUAL);
    } else if (depth_test_mode == "greater") {
        value = PipelineSettingParamType(DepthTestMode::GREATER);
    } else if (depth_test_mode == "greater_equal") {
        value = PipelineSettingParamType(DepthTestMode::GREATER_EQUAL);
    } else if (depth_test_mode == "always") {
        value = PipelineSettingParamType(DepthTestMode::ALWAYS);
    } else {
        throw RuntimeError(std::format("不支持的深度测试方法：\"{}\"", depth_test_mode.asString()));
    }
    mat->set_pipeline_setting("_depth_test", value);
}
NEXT2:

    // 材质参数
    for (auto param_iter = material_desc["parameters"].begin(); param_iter != material_desc["parameters"].end();
         ++param_iter) {
        mat->set_param(param_iter.name(), parse_material_parameters(param_iter->asString()));
    }

    // 纹理
    for (auto sampler_iter = material_desc["samplers"].begin(); sampler_iter != material_desc["samplers"].end();
         ++sampler_iter) {
        const std::string &name = sampler_iter.name();
        const std::string &texture = sampler_iter->asString();
        mat->set_texture(name, resources.load_resource<GLTexture>(texture));
    }

    return mat;
};

} // namespace Goonya
