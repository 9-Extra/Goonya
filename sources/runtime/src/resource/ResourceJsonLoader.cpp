#include "ResourceJsonLoader.h"

#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <vector>

#include "GraphicsResourceBuilder.h"
#include "function/renderer/RenderResource.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/Texture.h"
#include "platform/read_file.h"
#include "runtime/GoonyaException.h"

#include "glTFLoader.h"

namespace Goonya {
namespace Resource {

void load_json(const std::filesystem::path &path) {
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        if (!file) {
            throw RuntimeError(std::format("资源文件{}未找到", path.string()));
        }
        reader.parse(file, json, false);
    }

    std::filesystem::path base_dir = std::filesystem::absolute(path.parent_path()); // 包含此json文件的文件夹

    if (json.isMember("shader")) {
        for (const auto &key : json["shader"].getMemberNames()) {
            const Json::Value &shader_desc = json["shader"][key];
            const Json::Value &shader_sources = shader_desc["sources"];
            
            Graphics::UberShaderDesc desc{
                .vs_src = read_whole_file(base_dir / shader_sources["vertex_shader"].asString()),
                .ps_src = read_whole_file(base_dir / shader_sources["pixel_shader"].asString())
            };

            for(const auto& group: shader_desc["global_variants"]){
                std::vector<std::string> desc_group;
                for(const auto& key: group){
                    desc_group.emplace_back(key.asString());
                }
                desc.global_variant_keys.emplace_back(std::move(desc_group));
            }

            for(const auto& group: shader_desc["local_variants"]){
                std::vector<std::string> desc_group;
                for(const auto& key: group){
                    desc_group.emplace_back(key.asString());
                }
                desc.local_variant_keys.emplace_back(std::move(desc_group));
            }

            Graphics::resources.add_shader(key, std::move(desc));
        }
    }

    if (json.isMember("texture")) {
        for (const auto &key : json["texture"].getMemberNames()) {
            const Json::Value &texture_desc = json["texture"][key];
            Graphics::Texture2DDesc desc = {
                .path = base_dir / texture_desc["image"].asString()
            };

            if (texture_desc.isMember("is_color")) {
                desc.is_srgb = texture_desc["is_color"].asBool();
            }
            if (texture_desc.isMember("filter_mode")) {
                if (texture_desc["filter_mode"] == "point") {
                    desc.filter_mode = Graphics::TextureFilterMode::NEAREST;
                } else if (texture_desc["filter_mode"] == "bilinear") {
                    desc.filter_mode = Graphics::TextureFilterMode::BILINEAR;
                } else if (texture_desc["filter_mode"] == "trilinear") {
                    desc.filter_mode = Graphics::TextureFilterMode::TRILINEAR;
                } else {
                    throw RuntimeError(std::format("未知的纹理过滤模式：{}", texture_desc["filter_mode"].asString()));
                }
            }

            if (texture_desc.isMember("warp_mode")) {
                if (texture_desc["warp_mode"] == "repeat") {
                    desc.warp_mode = Graphics::TextureWarpMode::REPEAT;
                } else if (texture_desc["warp_mode"] == "clamp") {
                    desc.warp_mode = Graphics::TextureWarpMode::ClAMP;
                } else if (texture_desc["warp_mode"] == "mirror") {
                    desc.warp_mode = Graphics::TextureWarpMode::MIRROR;
                } else {
                    throw RuntimeError(std::format("未知的纹理重复模式：{}", texture_desc["warp_mode"].asString()));
                }
            }

            Graphics::resources.add_texture(key, desc);
        }
    }

    if (json.isMember("cubemap")) {
        for (const auto &key : json["cubemap"].getMemberNames()) {
            const Json::Value &cubemap_desc = json["cubemap"][key];
            Graphics::TextureCubeMapDesc desc{
                .path = {base_dir / cubemap_desc["px"].asString(), base_dir / cubemap_desc["nx"].asString(),
                         base_dir / cubemap_desc["py"].asString(), base_dir / cubemap_desc["ny"].asString(),
                         base_dir / cubemap_desc["pz"].asString(), base_dir / cubemap_desc["nz"].asString()}};

            if (cubemap_desc.isMember("is_color")) {
                desc.is_srgb = cubemap_desc["is_color"].asBool();
            }

            if (cubemap_desc.isMember("filter_mode")) {
                if (cubemap_desc["filter_mode"] == "point") {
                    desc.filter_mode = Graphics::TextureFilterMode::NEAREST;
                } else if (cubemap_desc["filter_mode"] == "bilinear") {
                    desc.filter_mode = Graphics::TextureFilterMode::BILINEAR;
                } else if (cubemap_desc["filter_mode"] == "bilinear") {
                    desc.filter_mode = Graphics::TextureFilterMode::TRILINEAR;
                } else {
                    throw RuntimeError(std::format("未知的纹理过滤模式：{}", cubemap_desc["filter_mode"].asString()));
                }
            }

            if (cubemap_desc.isMember("warp_mode")) {
                if (cubemap_desc["warp_mode"] == "repeat") {
                    desc.warp_mode = Graphics::TextureWarpMode::REPEAT;
                } else if (cubemap_desc["warp_mode"] == "clamp") {
                    desc.warp_mode = Graphics::TextureWarpMode::ClAMP;
                } else if (cubemap_desc["warp_mode"] == "mirror") {
                    desc.warp_mode = Graphics::TextureWarpMode::MIRROR;
                } else {
                    throw RuntimeError(std::format("未知的纹理重复模式：{}", cubemap_desc["warp_mode"].asString()));
                }
            }
            Graphics::resources.add_cubemap(key, desc);
        }
    }

    if (json.isMember("gltf")) {
        for (const auto &key : json["gltf"].getMemberNames()) {
            load_gltf(key, base_dir / json["gltf"][key]["path"].asString());
        }
    }

    // materials
    for (const auto &key : json["materials"].getMemberNames()) {
        const Json::Value &material_desc = json["materials"][key];
        
        Resource::MaterialBuilder mat_builder(material_desc["uber_shader"].asString());
 
        for (const auto &key : material_desc["variant_keys"]) {
            mat_builder.set_variant_key(key.asString());
        }

        const Json::Value &config = material_desc["config"];
        if (config.isMember("cull_mode")) {
            const Json::Value &cull_mode = config["cull_mode"];
            if (cull_mode == "front") {
                mat_builder.set_cull_mode(Graphics::CullFaceMode::FRONT);
            } else if (cull_mode == "back") {
                mat_builder.set_cull_mode(Graphics::CullFaceMode::BACK);
            } else if (cull_mode == "front_back") {
                mat_builder.set_cull_mode(Graphics::CullFaceMode::FRONT_AND_BACK);
            } else {
                throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", cull_mode.asString()));
            }
        }

        if (config.isMember("depth_func")) {
            const Json::Value &depth_test_mode = config["depth_func"];
            if (depth_test_mode == "never") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::NEVER);
            } else if (depth_test_mode == "less") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::LESS);
            } else if (depth_test_mode == "less_equal") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::LESS_EQUAL);
            } else if (depth_test_mode == "greater") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::GREATER);
            } else if (depth_test_mode == "greater_equal") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::GREATER_EQUAL);
            } else if (depth_test_mode == "always") {
                mat_builder.set_depth_test_mode(Graphics::DepthTestMode::ALWAYS);
            } else {
                throw RuntimeError(std::format("不支持的深度测试方法：\"{}\"", depth_test_mode.asString()));
            }
        }

        for (const Json::Value &uniform_json : material_desc["constants"]) {
            mat_builder.add_parameter(uniform_json["name"].asString(), uniform_json["vaule"].asUInt());
        }

        for (const Json::Value &sampler_json : material_desc["samplers"]) {
            mat_builder.add_sampler(sampler_json["name"].asString(), sampler_json["texture_key"].asString());
        }

        Graphics::resources.add_material(key, mat_builder.build());
    }
}

} // namespace Resource
} // namespace Goonya