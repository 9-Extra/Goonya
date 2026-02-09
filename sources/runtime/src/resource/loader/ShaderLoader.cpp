#include "ShaderLoader.h"

#include "core/as_u8string.h"
#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "function/renderer/PipelineLayout.h"
#include "function/renderer/UberShader.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/read_file.h"
#include "resource/ResMng.h"
#include "resource/loader/MaterialParameterParser.h"
#include "runtime/GAssert.h"

#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <regex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Goonya {

enum class Section {
    None = 0,
    Common = 1,
    Vertex = 2,
    Frag = 3,

    MAX
};

class ShaderParser {
private:
    std::array<std::reference_wrapper<const std::filesystem::path>, 2> include_paths;

    std::string_view g_shader_source;
    std::string sections_code[std::to_underlying(Section::MAX)];

    UberShaderDesc desc;

    Section current_section = Section::None;
    const char *current_section_begin = nullptr;

    void append_source(Section section, std::string_view source) {
        GN_ASSERT(section != Section::None);
        sections_code[std::to_underlying(section)].append(source);
    }

    void switch_section(Section new_section, const char *end_pos) {
        if (current_section != Section::None && current_section_begin != end_pos) {
            append_source(current_section, std::string_view(current_section_begin, end_pos));
        }
        current_section = new_section;
        current_section_begin = end_pos;
    }

    void skip(const char *start_pos, const char *end_pos) {
        if (current_section != Section::None && current_section_begin != start_pos) {
            append_source(current_section, std::string_view(current_section_begin, start_pos));
        }
        current_section_begin = end_pos;
    }

    void parse() {
        const std::regex pat = std::regex("#\\s*(.+?)\\s+(.+?)\\s*[\r\n]");
        using RegexIter = std::regex_iterator<std::string_view::const_iterator>;
        using Match = std::match_results<std::string_view::const_iterator>;
        for (RegexIter it(g_shader_source.begin(), g_shader_source.end(), pat); it != RegexIter(); ++it) {
            const std::string_view full_match = std::string_view((*it)[0].first, (*it)[0].second);
            const std::string_view instruction = std::string_view((*it)[1].first, (*it)[1].second);
            const std::string_view content = std::string_view((*it)[2].first, (*it)[2].second);
            // LOG_TRACE("解析指令{} {}", instruction, content);
            if (instruction == "name") {
                skip(full_match.begin(), full_match.end());
                desc.name = content;
            } else if (instruction == "texture") {
                skip(full_match.begin(), full_match.end());
                // skybox_specular_texture="buildin:missing_texture"
                const std::regex pat = std::regex(R"----(^(\w+)\s*=\s*"(.+)")----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    const std::string_view texture_key = std::string_view(match[1].first, match[1].second);
                    const std::string_view resource_key = std::string_view(match[2].first, match[2].second);
                    desc.textures.emplace(texture_key, resources.load_resource<GLTexture>(resource_key));
                } else {
                    LOG_ERROR("纹理指令#texture格式错误: {}", content);
                }
            } else if (instruction == "setting") {
                skip(full_match.begin(), full_match.end());
                // #setting cull_mode="off"
                const std::regex pat = std::regex(R"----(^(\w+)\s*=\s*"(\w+)")----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    const std::string_view setting_key = std::string_view(match[1].first, match[1].second);
                    const std::string_view setting_value = std::string_view(match[2].first, match[2].second);
                    if (setting_key == "cull_mode") {
                        if (setting_value == "off") {
                            desc.pipeline_setting.cull_mode = CullFaceMode::DISABLE;
                        } else if (setting_value == "front") {
                            desc.pipeline_setting.cull_mode = CullFaceMode::FRONT;
                        } else if (setting_value == "back") {
                            desc.pipeline_setting.cull_mode = CullFaceMode::BACK;
                        } else {
                            LOG_ERROR("未知的cull_mode: {}", setting_key);
                        }
                    }
                    if (setting_key == "depth_test") {
                        if (setting_value == "off") {
                            desc.pipeline_setting.depth_test = DepthTestMode::DISABLE;
                        } else if (setting_value == "less") {
                            desc.pipeline_setting.depth_test = DepthTestMode::LESS;
                        } else if (setting_value == "less_equal") {
                            desc.pipeline_setting.depth_test = DepthTestMode::LESS_EQUAL;
                        } else if (setting_value == "greater") {
                            desc.pipeline_setting.depth_test = DepthTestMode::GREATER;
                        } else if (setting_value == "greater_equal") {
                            desc.pipeline_setting.depth_test = DepthTestMode::GREATER_EQUAL;
                        } else if (setting_value == "equal") {
                            desc.pipeline_setting.depth_test = DepthTestMode::EQUAL;
                        } else if (setting_value == "not_equal") {
                            desc.pipeline_setting.depth_test = DepthTestMode::NOT_EQUAL;
                        } else if (setting_value == "never") {
                            desc.pipeline_setting.depth_test = DepthTestMode::NEVER;
                        } else if (setting_value == "always") {
                            desc.pipeline_setting.depth_test = DepthTestMode::ALWAYS;
                        } else {
                            LOG_ERROR("未知的depth_test模式: {}", setting_key);
                        }
                    }
                } else {
                    LOG_ERROR("渲染管线设置指令#setting格式错误: {}", content);
                }
            } else if (instruction == "param") {
                skip(full_match.begin(), full_match.end());
                // #param color_permutation=vec3(1, 1, 1)
                const std::regex pat = std::regex(R"----(^(\w+)\s*=\s*(.*))----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    const std::string_view param_name = std::string_view(match[1].first, match[1].second);
                    const MaterialParameter param_value =
                        parse_material_parameters(std::string_view(match[2].first, match[2].second));
                    desc.parameters.emplace(param_name, param_value);
                } else {
                    LOG_ERROR("参数指令#param格式错误: {}", content);
                }
            } else if (instruction == "local_variant") {
                skip(full_match.begin(), full_match.end());
                // #local_variant _ USE_NORMAL
                const std::regex pat = std::regex(R"----(\w+)----");
                std::vector<std::string> local_variant_keys;
                for (RegexIter it(content.begin(), content.end(), pat); it != RegexIter(); ++it) {
                    const std::string_view key = std::string_view((*it)[0].first, (*it)[0].second);
                    if (key != "_") {
                        local_variant_keys.emplace_back(key);
                    } else {
                        local_variant_keys.emplace_back("");
                    }
                }
                desc.local_variant_keys.emplace_back(std::move(local_variant_keys));
            } else if (instruction == "global_variant") {
                skip(full_match.begin(), full_match.end());
                // #global_variant WWWWW
                desc.global_variant_keys.emplace_back(content);
            } else if (instruction == "section") {
                if (content == "common") {
                    switch_section(Section::Common, full_match.begin());
                } else if (content == "vertex") {
                    switch_section(Section::Vertex, full_match.begin());
                } else if (content == "fragment") {
                    switch_section(Section::Frag, full_match.begin());
                } else if (content == "end") {
                    if (current_section == Section::None) {
                        LOG_ERROR("段落结束指令#end不能在段落外部使用");
                    }
                    switch_section(Section::None, full_match.begin());
                } else {
                    LOG_ERROR("未知着色器段落#section: {}", content);
                    switch_section(Section::None, full_match.begin());
                }
                skip(full_match.begin(), full_match.end());
            } else if (instruction == "include") {
                skip(full_match.begin(), full_match.end());
                if (current_section == Section::None) {
                    LOG_ERROR("包含指令#include只能在段落内部使用");
                    break;
                }
                // todo: include的文件内部的#include也要处理
                const std::regex pat = std::regex(R"----("(.+?)")----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    std::filesystem::path file = as_u8string_view(std::string_view(match[1].first, match[1].second));
                    bool success = false;
                    for (const std::filesystem::path &parent : include_paths) {
                        std::filesystem::path include_file = parent / file;
                        if (std::filesystem::is_regular_file(include_file)) {
                            append_source(current_section, read_whole_file(include_file));
                            append_source(current_section, "\n"); // 包含文件内容后添加一个换行符，避免和后续代码合并
                            success = true;
                            break;
                        }
                    }
                    if (!success) {
                        std::string err_info;
                        std::format_to(std::back_inserter(err_info), "找不到包含文件\"{}\"", file);
                        for (const std::filesystem::path &parent : include_paths) {
                            std::format_to(std::back_inserter(err_info), "\n    搜索路径: {}", parent / file);
                        }
                        LOG_ERROR(err_info);
                    }
                } else {
                    LOG_ERROR("包含指令#include格式错误: {}", content);
                }

            } else {
                // 其他指令，比如#version是glsl原本的东西
                if (current_section == Section::None) {
                    LOG_ERROR("未知指令\"{} {}\"，GLSL原指令只能在段落内部使用", instruction, content);
                } else {
                    switch_section(current_section, full_match.end());
                }
            }
        }

        switch_section(Section::None, g_shader_source.end());
    }
    std::string generate_material_uniform_block() const {
        if (desc.parameters.empty()) {
            return ""; // 空的uniform block会导致错误，没有材质参数时就不生成
        }
        std::string result;
        auto out = std::back_inserter(result);
        std::format_to(out, "layout(binding = {}) uniform per_material {{\n", PER_MATERIAL_UNIFORM_BINDING);

        for (auto &&[name, type_value] : desc.parameters) {
            std::string_view type_name = get_type_name_glsl(type_value);
            GN_ASSERT_MSG(!type_name.empty(), "无效的材质参数类型{}", type_value.index());
            std::format_to(out, "\t{} {};\n", type_name, name);
        }

        std::format_to(out, "}};\n");

        return result;
    }

public:
    explicit ShaderParser(std::string_view g_shader_source, const std::filesystem::path &source_path,
                          const std::filesystem::path &root_path)
        : include_paths{source_path, root_path}, g_shader_source(g_shader_source) {}

    UberShaderDesc get_desc() {
        parse();

        // 材质参数块加入到Common段落前方
        sections_code[std::to_underlying(Section::Common)] =
            generate_material_uniform_block() + sections_code[std::to_underlying(Section::Common)];

        desc.vs_src = std::format("{}\n{}\n{}\n{}", "#version 440 core\n#define VERTEX_SHADER\n#pragma GYA_INJECT\n",
                                  sections_code[std::to_underlying(Section::Common)],
                                  sections_code[std::to_underlying(Section::Vertex)], R"(void main() {vert();})");

        desc.ps_src = std::format("{}\n{}\n{}\n{}", "#version 440 core\n#define FRAGMENT_SHADER\n#pragma GYA_INJECT\n",
                                  sections_code[std::to_underlying(Section::Common)],
                                  sections_code[std::to_underlying(Section::Frag)], R"(void main() {frag();})");

        return std::move(desc);
    }
};

Ref<Resource> ShaderLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                 const Json::Value &content) {

    const Json::Value &shader_desc = content;

    std::filesystem::path g_shader_path = base_dir / as_u8string_view(shader_desc["source"].asString());
    std::string g_shader_source = read_whole_file(g_shader_path);

    std::filesystem::path parent_path =
        g_shader_path.parent_path(); // 因为GShaderParser内部使用引用，所以必须有临时变量
    ShaderParser parser(g_shader_source, parent_path, resources.get_root_dir());

    return create_ref<UberShader>(parser.get_desc());
}

} // namespace Goonya
