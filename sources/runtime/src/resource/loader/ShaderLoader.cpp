#include "ShaderLoader.h"

#include "core/as_u8string.h"
#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "function/renderer/PipelineLayout.h"
#include "function/renderer/RPriority.h"
#include "function/renderer/UberShader.h"
#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/read_file.h"
#include "resource/ResMng.h"
#include "resource/loader/MaterialParameterParser.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <array>
#include <charconv>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <regex>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Goonya {

enum class Section {
    None = 0,
    Common = 1,
    Vertex = 2,
    Frag = 3,
    Compute = 4,

    MAX
};

// #image指令的access参数：r / w / rw
static uint32_t parse_image_access(std::string_view access) noexcept {
    if (access == "r") {
        return GL_READ_ONLY;
    }
    if (access == "w") {
        return GL_WRITE_ONLY;
    }
    if (access == "rw") {
        return GL_READ_WRITE;
    }
    return 0;
}

// #image指令的format参数，使用GLSL图像格式限定符的名称，为了保证硬件寻址效率所以不支持3通道rgb格式，气笑了
// deepseek神力
static uint32_t parse_image_format(std::string_view format) noexcept {
    static const std::unordered_map<std::string_view, uint32_t> FORMAT_MAP = {
        {"rgba32f", GL_RGBA32F},           {"rgba16f", GL_RGBA16F},
        {"rg32f", GL_RG32F},               {"rg16f", GL_RG16F},
        {"r11f_g11f_b10f", GL_R11F_G11F_B10F}, {"r32f", GL_R32F},
        {"r16f", GL_R16F},                 {"rgba16", GL_RGBA16},
        {"rgb10_a2", GL_RGB10_A2},         {"rgba8", GL_RGBA8},
        {"rg16", GL_RG16},                 {"rg8", GL_RG8},
        {"r16", GL_R16},                   {"r8", GL_R8},
        {"rgba16_snorm", GL_RGBA16_SNORM}, {"rgba8_snorm", GL_RGBA8_SNORM},
        {"rg16_snorm", GL_RG16_SNORM},     {"rg8_snorm", GL_RG8_SNORM},
        {"r16_snorm", GL_R16_SNORM},       {"r8_snorm", GL_R8_SNORM},
        {"rgba32i", GL_RGBA32I},           {"rgba16i", GL_RGBA16I},
        {"rgba8i", GL_RGBA8I},             {"rg32i", GL_RG32I},
        {"rg16i", GL_RG16I},               {"rg8i", GL_RG8I},
        {"r32i", GL_R32I},                 {"r16i", GL_R16I},
        {"r8i", GL_R8I},                   {"rgba32ui", GL_RGBA32UI},
        {"rgba16ui", GL_RGBA16UI},         {"rgb10_a2ui", GL_RGB10_A2UI},
        {"rgba8ui", GL_RGBA8UI},           {"rg32ui", GL_RG32UI},
        {"rg16ui", GL_RG16UI},             {"rg8ui", GL_RG8UI},
        {"r32ui", GL_R32UI},               {"r16ui", GL_R16UI},
        {"r8ui", GL_R8UI},
    };
    if (auto iter = FORMAT_MAP.find(format); iter != FORMAT_MAP.end()) {
        return iter->second;
    }
    return 0;
}

class ShaderParser {
private:
    std::array<std::reference_wrapper<const std::filesystem::path>, 2> include_paths;

    std::string_view g_shader_source;
    std::string sections_code[std::to_underlying(Section::MAX)];

    UberShaderDesc desc;

    Section current_section = Section::None;
    const char *current_section_begin = nullptr;

    std::string &get_section(Section section) noexcept { return sections_code[std::to_underlying(section)]; }

    void append_source(Section section, std::string_view source) {
        GN_ASSERT(section != Section::None);
        get_section(section).append(source);
    }

    void switch_section(Section new_section, const char *end_pos) {
        if (current_section != Section::None && current_section_begin != end_pos) {
            append_source(current_section, std::string_view(current_section_begin, end_pos));
        }
        current_section = new_section;
        current_section_begin = end_pos;
    }

    void skip(std::string_view view) { skip(view.data(), view.data() + view.size()); }

    void skip(const char *start_pos, const char *end_pos) {
        if (current_section != Section::None && current_section_begin != start_pos) {
            append_source(current_section, std::string_view(current_section_begin, start_pos));
        }
        current_section_begin = end_pos;
    }

    void parse() {
        const std::regex pat = std::regex("#\\s*(.+?)\\s+(.+?)\\s*(//.*?|/\\*.*?|)[\r\n]");
        using RegexIter = std::regex_iterator<std::string_view::const_iterator>;
        using Match = std::match_results<std::string_view::const_iterator>;
        for (RegexIter it(g_shader_source.begin(), g_shader_source.end(), pat); it != RegexIter(); ++it) {
            const std::string_view full_match = std::string_view((*it)[0].first, (*it)[0].second);
            const std::string_view instruction = std::string_view((*it)[1].first, (*it)[1].second);
            const std::string_view content = std::string_view((*it)[2].first, (*it)[2].second);
            // LOG_TRACE("解析指令#{} {}", instruction, content);
            if (instruction == "name") {
                skip(full_match);
                desc.name = content;
            } else if (instruction == "category") {
                skip(full_match);
                // #category compute
                if (content == "compute") {
                    desc.category = ShaderCategory::COMPUTE;
                } else if (content == "graphics") {
                    desc.category = ShaderCategory::GRAPHICS;
                } else {
                    LOG_ERROR("未知的着色器类型: {}", content);
                }
            } else if (instruction == "texture") {
                skip(full_match);
                // #texture skybox_specular_texture="buildin:missing_texture"
                const std::regex pat = std::regex(R"----(^(\w+)\s*=\s*"(.+)")----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    const std::string_view texture_key = std::string_view(match[1].first, match[1].second);
                    const std::string_view resource_key = std::string_view(match[2].first, match[2].second);
                    desc.textures.emplace(texture_key, resources.load_resource<GLTexture>(resource_key));
                } else {
                    LOG_ERROR("纹理指令#texture格式错误: {}", content);
                }
            } else if (instruction == "image") {
                skip(full_match);
                // #image output_image access="rw" format="rgba32f"
                // access: r只读 / w只写 / rw读写；format使用GLSL图像格式限定符的名称
                // 图像的访问模式和格式无法通过OpenGL反射获取，必须在这里声明
                const std::regex pat = std::regex(R"----(^(\w+)\s+access\s*=\s*"(\w+)"\s+format\s*=\s*"(\w+)")----");
                Match match;
                if (std::regex_match(content.begin(), content.end(), match, pat)) {
                    const std::string_view image_name = std::string_view(match[1].first, match[1].second);
                    const std::string_view access = std::string_view(match[2].first, match[2].second);
                    const std::string_view format = std::string_view(match[3].first, match[3].second);
                    uint32_t gl_access = parse_image_access(access);
                    uint32_t gl_type = parse_image_format(format);
                    if (gl_access == 0) {
                        LOG_ERROR("图像指令#image的access值无效: {}（仅支持r/w/rw）", access);
                    } else if (gl_type == 0) {
                        LOG_ERROR("图像指令#image的format值无效: {}", format);
                    } else {
                        desc.images.emplace_back(image_name, ImageDecl{gl_access, gl_type});
                    }
                } else {
                    LOG_ERROR("图像指令#image格式错误: {}", content);
                }
            } else if (instruction == "setting") {
                skip(full_match);
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

                    if (setting_key == "render_priority") {
                        std::underlying_type_t<RenderPriority> p;
                        if (std::from_chars(setting_value.data(), setting_value.data() + setting_value.size(), p).ec ==
                            std::errc{}) {
                            desc.render_priority = RenderPriority(p);
                        } else {
                            LOG_ERROR("无法解析priority的值: {}", setting_key);
                        }
                    }

                    if (setting_key == "write_mask") {
                        desc.pipeline_setting.write_red = false;
                        desc.pipeline_setting.write_green = false;
                        desc.pipeline_setting.write_blue = false;
                        desc.pipeline_setting.write_alpha = false;
                        desc.pipeline_setting.write_stencil = false;
                        desc.pipeline_setting.write_depth = false;
                        for (char c : setting_value) {
                            switch (c) {
                            case 'r':
                                desc.pipeline_setting.write_red = true;
                                break;
                            case 'g':
                                desc.pipeline_setting.write_green = true;
                                break;
                            case 'b':
                                desc.pipeline_setting.write_blue = true;
                                break;
                            case 'a':
                                desc.pipeline_setting.write_alpha = true;
                                break;
                            case 'd':
                                desc.pipeline_setting.write_depth = true;
                                break;
                            case 's':
                                desc.pipeline_setting.write_stencil = true;
                                break;
                            case '0':
                                break;
                            default:
                                LOG_ERROR("未知的write_mask字符: {}", c);
                                break;
                            }
                        }
                    }
                } else {
                    LOG_ERROR("渲染管线设置指令#setting格式错误: {}", content);
                }
            } else if (instruction == "param") {
                skip(full_match);
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
                skip(full_match);
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
                skip(full_match);
                // #global_variant WWWWW
                desc.global_variant_keys.emplace_back(content);
            } else if (instruction == "section") {
                if (content == "common") {
                    switch_section(Section::Common, full_match.data());
                } else if (content == "vertex") {
                    switch_section(Section::Vertex, full_match.data());
                } else if (content == "fragment") {
                    switch_section(Section::Frag, full_match.data());
                } else if (content == "compute") {
                    switch_section(Section::Compute, full_match.data());
                } else if (content == "end") {
                    if (current_section == Section::None) {
                        LOG_ERROR("段落结束指令#end不能在段落外部使用");
                    }
                    switch_section(Section::None, full_match.data());
                } else {
                    LOG_ERROR("未知着色器段落#section: {}", content);
                    switch_section(Section::None, full_match.data());
                }
                skip(full_match);
            } else if (instruction == "include") {
                skip(full_match);
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
                    switch_section(current_section, full_match.data() + full_match.size());
                }
            }
        }

        switch_section(Section::None, g_shader_source.data() + g_shader_source.size());
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
        desc.category = ShaderCategory::GRAPHICS; // 默认为光栅化管线
        parse();

        // 材质参数块加入到Common段落前方
        get_section(Section::Common) =
            generate_material_uniform_block() + sections_code[std::to_underlying(Section::Common)];

        if (desc.category == ShaderCategory::GRAPHICS) {
            if (!get_section(Section::Compute).empty()) {
                LOG_WARN("光栅化着色器内包含计算着色器块，内容被丢弃");
            }
            desc.vs_src =
                std::format("{}\n{}\n{}\n{}", "#version 440 core\n#define VERTEX_SHADER\n#pragma GYA_INJECT\n",
                            get_section(Section::Common), get_section(Section::Vertex), R"(void main() {vert();})");

            desc.ps_src =
                std::format("{}\n{}\n{}\n{}", "#version 440 core\n#define FRAGMENT_SHADER\n#pragma GYA_INJECT\n",
                            get_section(Section::Common), get_section(Section::Frag), R"(void main() {frag();})");
        } else if (desc.category == ShaderCategory::COMPUTE) {
            if (!get_section(Section::Vertex).empty()) {
                LOG_WARN("计算着色器内包含顶点着色器块，内容被丢弃");
            }
            if (!get_section(Section::Frag).empty()) {
                LOG_WARN("计算着色器内包含像素着色器块，内容被丢弃");
            }
            desc.vs_src = std::format("{}\n{}\n{}\n", "#version 440 core\n#define COMPUTE_SHADER\n#pragma GYA_INJECT\n",
                                      get_section(Section::Common), get_section(Section::Compute));
        } else {
            throw RuntimeError("未知的着色器种类");
        }

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
