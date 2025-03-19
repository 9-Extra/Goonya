#include "ResourceJsonLoader.h"

#include <cstddef>
#include <fstream>
#include <json/json.h>

#include "GraphicsResourceBuilder.h"
#include "function/renderer/RenderResource.h"
#include "platform/graphics/Texture.h"
#include "resource/resources.h"
#include "runtime/GoonyaException.h"

#include "glTFLoader.h"

namespace Goonya {
namespace Resource {

void load_json(const std::string &path) {
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        if (!file) {
            throw RuntimeError(std::format("资源文件{}未找到", path));
        }
        reader.parse(file, json, false);
    }

    std::string base_dir;
    if (size_t it = path.find_last_of("/\\"); it != std::string::npos) {
        base_dir = path.substr(0, it + 1); // 包含'/'
    } else {
        base_dir = ""; // 可能在同一目录下
    }

    if (json.isMember("shader")) {
        for (const auto &key : json["shader"].getMemberNames()) {
            const Json::Value &shader_desc = json["shader"][key];
            Graphics::resources.add_shader(key, base_dir + shader_desc["vs_path"].asString(),
                                           base_dir + shader_desc["ps_path"].asString());
        }
    }

    if (json.isMember("texture")) {
        for (const auto &key : json["texture"].getMemberNames()) {
            const Json::Value &texture_desc = json["texture"][key];
            Texture2DDesc desc = {
                .path = base_dir + texture_desc["image"].asString()
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
            TextureCubeMapDesc desc{
                .path = {base_dir + cubemap_desc["px"].asString(), base_dir + cubemap_desc["nx"].asString(),
                         base_dir + cubemap_desc["py"].asString(), base_dir + cubemap_desc["ny"].asString(),
                         base_dir + cubemap_desc["pz"].asString(), base_dir + cubemap_desc["nz"].asString()}};

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
            load_gltf(key, base_dir + json["gltf"][key]["path"].asString());
        }
    }

    // materials
    for (const auto &key : json["materials"].getMemberNames()) {
        const Json::Value &material_desc = json["materials"][key];

        Resource::PSOBuilder pso_builder(material_desc["uber_shader"].asString());
        
        for (const auto &key : material_desc["variant_keys"]) {
            pso_builder.set_variant_key(key.asString());
        }

        const Json::Value &config = material_desc["config"];
        if (config.isMember("depth_func")) {
            pso_builder.set_depth_func(config["depth_func"].asString());
        }

        Resource::MaterialBuilder mat_builder(pso_builder.build());

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