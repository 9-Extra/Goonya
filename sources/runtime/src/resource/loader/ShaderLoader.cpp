#include "ShaderLoader.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/UberShader.h"
#include "platform/read_file.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

Ref<Resource> ShaderLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                 const Json::Value &content) {

    const Json::Value &shader_desc = content;

    const Json::Value &shader_sources = shader_desc["sources"];
    Graphics::UberShaderDesc desc{.vs_src = read_whole_file(base_dir / shader_sources["vertex_shader"].asString()),
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
        if (!Graphics::PipelineSettingSetter::is_pipeline_setting(key)){
            throw RuntimeError(std::format("未知渲染管线设置{}", key));
        }
        const Json::Value &value = shader_desc["pipeline_setting"][key];
        Graphics::PipelineSettingSetter::set_pipeline_setting(key, value.asInt(), desc.pipeline_setting);
    }

    return Ref<Graphics::UberShader>(new Graphics::UberShader(std::move(desc)));
}

} // namespace Goonya