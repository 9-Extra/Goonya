#include "MaterialLoader.h"

#include "core/RefCount.h"
#include "core/cgmath/vector.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/UberShader.h"
#include "resource/ResMng.h"
#include "rfl/enums.hpp"

#include <regex>

namespace Goonya {

/**
 * @brief 解析形如"vec3(1.0, 0, 2.0)"这样的材质参数
 *
 * @param mat_builder 材质Builder，解析后的参数添加到Builder
 * @param name 参数在着色器中的名称
 * @param parameter_string 字符串格式的参数
 */
static void parse_material_paramter(Ref<Graphics::Material> &material, const std::string &name,
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
            material->set_param(name, param);
        } else if (type_name == "vec3") {
            Vector3f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            assert(it == end);
            material->set_param(name, param);
        } else if (type_name == "vec4") {
            Vector4f param;
            for (float &i : param.v) {
                i = std::stof(it->str());
                ++it;
            }
            assert(it == end);
            material->set_param(name, param);
        } else if (type_name == "f32") {
            float param = std::stof(it->str());
            ++it;
            assert(it == end);
            material->set_param(name, param);
        } else if (type_name == "mat4") {
            Matrix4 param{
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()),
                std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str()), std::stof((it++)->str())};

            assert(it == end);
            material->set_param(name, param);
        } else {
            throw RuntimeError(std::format("着色器参数类型\"{}\"不支持", type_name));
        }

    } else {
        throw RuntimeError(std::format("着色器参数格式\"{}\"不正确", parameter_string));
    }
}

Ref<Resource> MateriaLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                  const Json::Value &content) {
    const Json::Value &material_desc = content;

    Ref<Graphics::Material> mat = create_ref<Graphics::Material>(
        resources.load_resource<Graphics::UberShader>(material_desc["uber_shader"].asString()).get());
    if (!mat) {
        throw RuntimeError("初始化材质失败");
    }

    for (const auto &variant_key : material_desc["variant_keys"]) {
        mat->set_local_variant_key(variant_key.asString());
    }

    const Json::Value &config = material_desc["config"];

    {
        Graphics::PipelineSettingParamType value;
        const Json::Value &cull_mode = config["cull_mode"];
        if (!cull_mode) {
            goto NEXT;
        } else if (cull_mode == "back") {
            value = Graphics::PipelineSettingParamType(Graphics::CullFaceMode::BACK);
        } else if (cull_mode == "front") {
            value = Graphics::PipelineSettingParamType(Graphics::CullFaceMode::FRONT);
        } else if (cull_mode == "front_back") {
            value = Graphics::PipelineSettingParamType(Graphics::CullFaceMode::FRONT_AND_BACK);
        } else {
            throw RuntimeError(std::format("不支持的面裁剪模式：\"{}\"", cull_mode.asString()));
        }
        mat->set_pipeline_setting("_cull_mode", value);
    }
NEXT: {
    Graphics::PipelineSettingParamType value;
    const Json::Value &depth_test_mode = config["depth_func"];
    if (!depth_test_mode) {
        goto NEXT2;
    } else if (depth_test_mode == "less") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::LESS);
    } else if (depth_test_mode == "never") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::NEVER);
    } else if (depth_test_mode == "less_equal") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::LESS_EQUAL);
    } else if (depth_test_mode == "greater") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::GREATER);
    } else if (depth_test_mode == "greater_equal") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::GREATER_EQUAL);
    } else if (depth_test_mode == "always") {
        value = Graphics::PipelineSettingParamType(Graphics::DepthTestMode::ALWAYS);
    } else {
        throw RuntimeError(std::format("不支持的深度测试方法：\"{}\"", depth_test_mode.asString()));
    }
    mat->set_pipeline_setting("_depth_test", value);
}
NEXT2:
    for (auto param_iter = material_desc["parameters"].begin(); param_iter != material_desc["parameters"].end();
         ++param_iter) {
        parse_material_paramter(mat, param_iter.name(), param_iter->asString());
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
            } else {
                throw RuntimeError(std::format("未知纹理类型\"{}\"", type_name.str()));
            }

            Ref<Graphics::Texture> texture = resources.load_resource<Graphics::Texture>(matches[2].str());
            if (!texture) {
                throw RuntimeError(std::format("加载纹理{}失败", matches[2].str()));
            }
            if (texture->get_type() != type) {
                throw RuntimeError(std::format("纹理类型不匹配，应为{}， 实为{}",
                                               rfl::enum_to_string(texture->get_type()), rfl::enum_to_string(type)));
            }
            mat->set_texture(name, texture);
        } else {
            throw RuntimeError(std::format("纹理参数格式\"{}\"不正确", texture));
        }
    }

    return mat;
};

} // namespace Goonya