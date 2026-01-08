#include "ShaderLoader.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/UberShader.h"
#include "platform/read_file.h"
#include "resource/ResMng.h"
#include "resource/loader/MaterialParameterParser.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

Ref<Resource> ShaderLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                 const Json::Value &content) {

    const Json::Value &shader_desc = content;

    const Json::Value &shader_sources = shader_desc["sources"];
    UberShaderDesc desc{.vs_src = read_whole_file(base_dir / shader_sources["vertex_shader"].asString()),
                        .ps_src = read_whole_file(base_dir / shader_sources["pixel_shader"].asString())};

    for (const auto &key : shader_desc["global_variants"]) {
        desc.global_variant_keys.emplace_back(key.asString());
    }

    for (const auto &group : shader_desc["local_variants"]) {
        std::vector<std::string> desc_group;
        for (const auto &variant_key : group) {
            desc_group.emplace_back(variant_key.asString());
        }
        desc.local_variant_keys.emplace_back(std::move(desc_group));
    }

    for (const auto &key : shader_desc["pipeline_setting"].getMemberNames()) {
        if (!PipelineSettingSetter::is_pipeline_setting(key)) {
            throw RuntimeError(std::format("未知渲染管线设置{}", key));
        }
        const Json::Value &value = shader_desc["pipeline_setting"][key];
        PipelineSettingSetter::set_pipeline_setting(key, value.asInt(), desc.pipeline_setting);
    }

    // 材质参数
    for (auto param_iter = shader_desc["parameters"].begin(); param_iter != shader_desc["parameters"].end();
         ++param_iter) {
        desc.parameters.emplace(param_iter.name(), parse_material_parameters(param_iter->asString()));
    }

    // 纹理
    for (auto sampler_iter = shader_desc["samplers"].begin(); sampler_iter != shader_desc["samplers"].end();
         ++sampler_iter) {
        const std::string &name = sampler_iter.name();
        const std::string &texture = sampler_iter->asString();
        Ref<GLTexture> tex = resources.load_resource<GLTexture>(texture);
        if (tex) {
            desc.textures.emplace(name, tex);
        } else {
            throw RuntimeError(std::format("元着色器必须制定默认纹理{}", name));
        }
    }

    return Ref<UberShader>(new UberShader(std::move(desc)));
}

} // namespace Goonya
