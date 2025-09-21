#include "ResMng.h"

#include "json/reader.h"
#include "json/value.h"
#include <cassert>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <regex>
#include <span>
#include <vector>

#include "GraphicsResourceBuilder.h"
#include "HardcodeAssets.h"
#include "core/format_exception.h"
#include "core/log/Log.h"
#include "platform/graphics/Mesh.h"
#include "platform/read_file.h"
#include "resource/glTFLoader.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

RenderResource resources; // Global

namespace fs = std::filesystem;

void RenderResource::init_buildin_resources() {
    // 部分硬编码的mesh
    std::span<const std::byte> plane_vertex_span = std::as_bytes(std::span(Assets::plane_vertices));
    Graphics::MeshDesc plane{Assets::plane_vertices_vertex_layout,
                             std::vector(plane_vertex_span.begin(), plane_vertex_span.end()), Assets::plane_indices,
                             Graphics::Topology::TRIANGLE};
    meshes.add("plane", std::move(plane));
    // 添加天空盒的mesh，因为格式不一样所以单独处理
    std::span<const std::byte> skybox_cube_vertex_span = std::as_bytes(std::span(Assets::skybox_cube_vertices));
    Graphics::MeshDesc skybox_cube{Assets::skybox_cube_vertex_layout,
                                   std::vector(skybox_cube_vertex_span.begin(), skybox_cube_vertex_span.end()),
                                   Assets::skybox_cube_indices, Graphics::Topology::TRIANGLE};
    meshes.add("skybox_cube", std::move(skybox_cube));
}

void RenderResource::scan() {
    assert(!resource_dir.empty());
    for (const fs::directory_entry &dir_entry :
         fs::recursive_directory_iterator(resource_dir, fs::directory_options::follow_directory_symlink)) {
        if (dir_entry.is_regular_file()) {
            const fs::path &path = dir_entry.path();
            std::u8string ext = path.extension().u8string();
            if (ext == u8".meta") {
                try {
                    try_load(path);
                } catch (const std::exception &e) {
                    LOG_WARN("加载资源{}时遇到错误：{}\n{}", path.generic_string(), e.what(), format_exception(e));
                }
            } else if (ext == u8".gltf") {
                Resource::load_gltf(path_to_key(path), path);
            }
        }
    }
}

static std::tuple<Graphics::TextureFilterMode, Graphics::TextureWarpMode>
parse_texture_profile(const Json::Value &texture_desc) {
    Graphics::TextureFilterMode filter_mode;
    Graphics::TextureWarpMode warp_mode;
    const Json::Value &filter_mode_name = texture_desc["filter_mode"];
    const Json::Value &warp_mode_name = texture_desc["warp_mode"];

    if (!filter_mode_name || filter_mode_name == "trilinear") {
        filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    } else if (filter_mode_name == "point") {
        filter_mode = Graphics::TextureFilterMode::BILINEAR;
    } else if (filter_mode_name == "bilinear") {
        filter_mode = Graphics::TextureFilterMode::NEAREST;
    } else {
        throw RuntimeError(std::format("未知的纹理过滤模式：{}", filter_mode_name.asString()));
    }

    if (!warp_mode_name || warp_mode_name == "repeat") {
        warp_mode = Graphics::TextureWarpMode::REPEAT;
    } else if (warp_mode_name == "clamp") {
        warp_mode = Graphics::TextureWarpMode::ClAMP;
    } else if (warp_mode_name == "mirror") {
        warp_mode = Graphics::TextureWarpMode::MIRROR;
    } else {
        throw RuntimeError(std::format("未知的纹理重复模式：{}", warp_mode_name.asString()));
    }

    return {filter_mode, warp_mode};
}

/**
 * @brief 解析形如"vec3(1.0, 0, 2.0)"这样的材质参数
 *
 * @param mat_builder 材质Builder，解析后的参数添加到Builder
 * @param name 参数在着色器中的名称
 * @param parameter_string 字符串格式的参数
 */
static void parse_material_paramter(Resource::MaterialBuilder &mat_builder, const std::string &name,
                                    const std::string &parameter_string) {
    // 解析形如"vec3(1.0, 0, 2.0)"这样的参数
    const std::regex pattern(R"(^\s*(\w+)\s*\((.*)\)$)");
    const std::regex number_pattern(R"(\s*([-+]?\d*\.?\d+\s*))");
    std::smatch matches;
    if (std::regex_match(parameter_string, matches, pattern)) {
        // matches[0] 是整个匹配的字符串
        std::string type_name = matches[1].str(); // 类型
        std::string numbers_str = matches[2].str();
        std::sregex_iterator it(numbers_str.begin(), numbers_str.end(), number_pattern);
        std::sregex_iterator end;

        if (type_name == "vec2") {
            Vector2f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            assert(it == end);
            mat_builder.add_parameter(name, param);
        } else if (type_name == "vec3") {
            Vector3f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            assert(it == end);
            mat_builder.add_parameter(name, param);
        } else if (type_name == "vec4") {
            Vector4f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            assert(it == end);
            mat_builder.add_parameter(name, param);
        } else if (type_name == "f32") {
            float param = std::stof(it->str());
            ++it;
            assert(it == end);
            mat_builder.add_parameter(name, param);
        } else if (type_name == "mat4") {
            Matrix4 param{
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str())};

            assert(it == end);
            mat_builder.add_parameter(name, param);
        } else {
            throw RuntimeError(std::format("着色器参数类型\"{}\"不支持", type_name));
        }

    } else {
        throw RuntimeError(std::format("着色器参数格式\"{}\"不正确", parameter_string));
    }
}

void RenderResource::try_load(const std::filesystem::path &path) {

    std::ifstream file(path, std::ios::binary | std::ios::in);
    if (!file) {
        throw RuntimeError("打开文件失败");
    }

    Json::Value meta;
    if (!Json::parseFromStream(Json::CharReaderBuilder(), file, &meta, nullptr)) {
        throw RuntimeError("解析Json出错");
    }
    const AssetKey &key = path_to_key(path);
    if (key.empty()) {
        throw RuntimeError("键未能正常生成");
    }

    std::filesystem::path base_dir = path.parent_path();
    const std::string &res_type = meta["type"].asString();
    const Json::Value &content = meta["content"];
    if (res_type == "Texture") {
        const Json::Value &texture_desc = content;

        Graphics::Texture2DDesc desc = {.path = base_dir / texture_desc["image"].asString()};

        desc.is_color = texture_desc.get("is_color", false).asBool();
        auto [filter_mode, warp_mode] = parse_texture_profile(texture_desc);
        desc.filter_mode = filter_mode;
        desc.warp_mode = warp_mode;

        resources.texture2ds.add(key, desc);
    } else if (res_type == "UberShader") {
        const Json::Value &shader_desc = content;

        const Json::Value &shader_sources = shader_desc["sources"];
        Graphics::UberShaderDesc desc{.vs_src = read_whole_file(base_dir / shader_sources["vertex_shader"].asString()),
                                      .ps_src = read_whole_file(base_dir / shader_sources["pixel_shader"].asString())};

        for (const auto &group : shader_desc["global_variants"]) {
            std::vector<std::string> desc_group;
            for (const auto &variant_key : group) {
                desc_group.emplace_back(variant_key.asString());
            }
            desc.global_variant_keys.emplace_back(std::move(desc_group));
        }

        for (const auto &group : shader_desc["local_variants"]) {
            std::vector<std::string> desc_group;
            for (const auto &variant_key : group) {
                desc_group.emplace_back(variant_key.asString());
            }
            desc.local_variant_keys.emplace_back(std::move(desc_group));
        }

        resources.add_shader(key, std::move(desc));
    } else if (res_type == "CubeMap") {
        const Json::Value &cubemap_desc = content;

        Graphics::TextureCubeMapDesc desc{
            .path = {base_dir / cubemap_desc["px"].asString(), base_dir / cubemap_desc["nx"].asString(),
                     base_dir / cubemap_desc["py"].asString(), base_dir / cubemap_desc["ny"].asString(),
                     base_dir / cubemap_desc["pz"].asString(), base_dir / cubemap_desc["nz"].asString()}};

        desc.is_color = cubemap_desc.get("is_color", false).asBool();
        auto [filter_mode, warp_mode] = parse_texture_profile(cubemap_desc);
        desc.filter_mode = filter_mode;
        desc.warp_mode = warp_mode;

        resources.cubemaps.add(key, desc);
    } else if (res_type == "Material") {
        const Json::Value &material_desc = content;

        Resource::MaterialBuilder mat_builder(material_desc["uber_shader"].asString());

        for (const auto &variant_key : material_desc["variant_keys"]) {
            mat_builder.set_variant_key(variant_key.asString());
        }

        const Json::Value &config = material_desc["config"];

        const Json::Value &cull_mode = config["cull_mode"];
        if (!cull_mode || cull_mode == "back") {
            mat_builder.set_cull_mode(Graphics::CullFaceMode::BACK);
        } else if (cull_mode == "front") {
            mat_builder.set_cull_mode(Graphics::CullFaceMode::FRONT);
        } else if (cull_mode == "front_back") {
            mat_builder.set_cull_mode(Graphics::CullFaceMode::FRONT_AND_BACK);
        } else {
            throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", cull_mode.asString()));
        }

        const Json::Value &depth_test_mode = config["depth_func"];
        if (!depth_test_mode || depth_test_mode == "less") {
            mat_builder.set_depth_test_mode(Graphics::DepthTestMode::LESS);
        } else if (depth_test_mode == "never") {
            mat_builder.set_depth_test_mode(Graphics::DepthTestMode::NEVER);
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

        for (auto param_iter = material_desc["parameters"].begin(); param_iter != material_desc["parameters"].end();
             ++param_iter) {
            parse_material_paramter(mat_builder, param_iter.name(), param_iter->asString());
        }

        for (auto sampler_iter = material_desc["samplers"].begin(); sampler_iter != material_desc["samplers"].end();
             ++sampler_iter) {
            const std::string &name = sampler_iter.name();
            const std::string &texture = sampler_iter->asString();
            const static std::regex pattern(R"(^\s*(\w+)\s*\((.+)\)$)");
            std::smatch matches;
            if (std::regex_match(texture, matches, pattern)) {
                Graphics::TextureType type = Graphics::TextureType::UNKNOWN;
                const auto type_name = matches[1];

                if (type_name.compare("texture2d") == 0) {
                    type = Graphics::TextureType::TEXTURE_2D;
                } else if (type_name.compare("cubemap") == 0) {
                    type = Graphics::TextureType::TEXTURE_CUBEMAP;
                }

                if (type != Graphics::TextureType::UNKNOWN) {
                    mat_builder.add_sampler(name, type, matches[2].str());
                } else {
                    throw RuntimeError(std::format("未知纹理类型\"{}\"", type_name.str()));
                }
            } else {
                throw RuntimeError(std::format("纹理参数格式\"{}\"不正确", texture));
            }
        }

        resources.materials.add(key, mat_builder.build());
    } else {
        throw RuntimeError(std::format("加载{}时遇到未知资源类型{}", path.generic_string(), res_type));
    }
}
} // namespace Goonya