#include "UberShader.h"

#include "core/RefCount.h"
#include "core/log/Log.h"
#include "function/renderer/PipelineLayout.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <cstdint>
#include <limits>

namespace Goonya {

GlobalVariantKeyCollect GLOBAL_VARIANT_KEY;

void LocalVariantKeyCollect::add_variant_key_group(std::vector<std::string> &&group_keys) {
    if (group_keys.size() < 2) {
        throw RuntimeError("每个组至少有2个成员");
    }

    if (variants_key_map.size() > std::numeric_limits<uint8_t>::max()) {
        throw RuntimeError("变体组太多");
    }

    uint8_t group_index = (uint8_t)variants_key_map.size();
    uint8_t index = 0;
    for (const std::string &key : group_keys) {
        if (!key.empty()) { // 略过空定义
            auto [_, inserted] = variants_key_map.emplace(key, std::make_tuple(group_index, index));
            if (!inserted) {
                throw RuntimeError("局部着色器变体键重复定义");
            }
        }
        index++;
    }

    uint32_t group_key_count = (uint32_t)group_keys.size();
    uint32_t next_group_base = variant_count;
    variants_key_groups.emplace_back(next_group_base, std::move(group_keys));
    variant_count *= group_key_count;
    GN_ASSERT(variant_count >= group_key_count); // 越界检测
}
void LocalVariantKeyCollect::get_variant_key_names(VariantCode code,
                                                   std::vector<std::string> &out_result) const noexcept {
    for (const auto &[base, group] : variants_key_groups) {
        const std::string &key = group[code / base % static_cast<uint32_t>(group.size())];
        if (!key.empty()) { // 空字符串视为不进行定义
            out_result.emplace_back(key);
        }
    }
}
bool LocalVariantKeyCollect::set_variant_code(VariantCode &code, std::string_view variant_key) const noexcept {
    if (auto iter = variants_key_map.find(variant_key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % static_cast<uint32_t>(group.size()); // 当前本组选择的变体下标
        if (current_index != index) {
            code += base * index - base * current_index;
            return true;
        }
        return false;
    } else {
        // 如果此键不存在，也返回false
        return false;
    }
}
bool LocalVariantKeyCollect::reset_variant_code(VariantCode &code, std::string_view variant_key) const noexcept {
    if (auto iter = variants_key_map.find(variant_key); iter != variants_key_map.end()) {
        auto [group_index, _] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        uint32_t current_index = code / base % static_cast<uint32_t>(group.size()); // 当前本组选择的变体下标
        if (current_index != 0) {
            code -= base * current_index;
            return true;
        }
        return false;
    } else {
        return false;
    }
}
bool LocalVariantKeyCollect::is_key_defined(VariantCode code, std::string_view key) const noexcept {
    if (auto iter = variants_key_map.find(key); iter != variants_key_map.end()) {
        auto [group_index, index] = iter->second;
        const auto &[base, group] = variants_key_groups[group_index];
        return code / base % static_cast<uint32_t>(group.size()) == index;
    }
    return false;
}

/**
 * @brief 将宏定义等代码插入到着色器中以生成对应变体
 * 插入的代码将替换着色器源码中的#pragma GYA_INJECT
 * @param src 着色器代码
 * @param variant_key
 * @return 生成的代码
 */
static std::string shader_source_inject(const std::string &src, const std::vector<std::string> &variant_key) {

    const std::string PATTERN = "#pragma GYA_INJECT";

    std::stringstream ss;
    size_t injection_point = src.find(PATTERN);
    if (injection_point == std::string::npos) {
        LOG_WARN("着色器中未找到\"{}\"，无法正确进行变体生成", PATTERN);
        return src;
    }
    ss << src.substr(0, injection_point);

    ss << "//------Combined Definitions---------\n";

    for (const std::string &key : variant_key) {
        ss << std::format("#define {}\n", key);
    }

    ss << "//------Combined Definition End------\n";

    ss << src.substr(injection_point + PATTERN.size());

    return ss.str();
}

UberShader::UberShader(UberShaderDesc &&desc) {
    GN_ASSERT(desc.category != ShaderCategory::NONE);
    this->display_name = std::move(desc.name);
    this->category = desc.category;
    this->vs_src = std::move(desc.vs_src);
    this->ps_src = std::move(desc.ps_src);
    this->pipeline_setting = desc.pipeline_setting;
    this->local_variant_key_collect = LocalVariantKeyCollect(std::move(desc.local_variant_keys));
    this->effective_global_key_mask = GLOBAL_VARIANT_KEY.get_shader_global_key_mask(desc.global_variant_keys);
    this->render_priority = desc.render_priority;

    // 纹理的单元由着色器中的layout(binding = N)决定，这里只记录#texture声明的默认纹理
    for (auto &&[name, default_texture] : desc.textures) {
        if (find_engine_texture(name) != nullptr) {
            throw RuntimeError(std::format("纹理\"{}\"是引擎纹理，不能通过#texture声明默认纹理", name));
        }
        if (!default_texture) {
            throw RuntimeError(std::format("纹理\"{}\"的默认纹理加载失败", name));
        }
        default_textures.emplace(name, std::move(default_texture));
        texture_units.emplace(name, TextureParameterInfo{}); // 单元和类型在编译变体时获取
    }

    // 图像单元同样由layout(binding = N)决定，#image指令只声明访问模式和格式
    for (auto &&[name, decl] : desc.images) {
        image_units.emplace(name, ImageParameterInfo{TextureType::UNKNOWN, std::nullopt, decl.gl_access, decl.gl_type});
    }

    // 立即编译变体码为0的版本用于反射
    Ref<GLShader> shader = query_variant(VariantCodeSet{0});

    GLShaderIntrospector introspector{shader.get()};

    // 反射获取着色器信息
    this->material_parameters = introspector.get_per_material_uniform_info();
    this->uniform_binding_info = introspector.get_uniform_binding_info();
    // 不同变体的纹理单元绑定可能不同，需要在每次编译时手动设置，保证每个变体的纹理单元绑定是一致的

    // 遍历所有参数，将.meta中定义的参数值赋值给材质参数
    for (auto &&[name, field] : material_parameters.fields) {
        auto iter = desc.parameters.find(name);
        if (iter != desc.parameters.end()) {
            if (field.type_and_default_value.index() != iter->second.index()) {
                throw RuntimeError(std::format("参数\"{}\"在着色器中定义为了不同的类型", name));
            }
            field.type_and_default_value = iter->second;
        } else {
            LOG_INFO("参数\"{}\"在着色器中未定义，将使用默认值", name);
        }
    }
}

Ref<GLShader> UberShader::query_variant(VariantCodeSet variant_code) {
    if (auto iter = shaders.find(variant_code); iter != shaders.end()) {
        // 缓存中已存在此变体，直接返回
        return iter->second;
    }

    // 获取global_code时，记得先进行effective_global_key_mask掩码操作
    GN_ASSERT(((!effective_global_key_mask) & variant_code.global_code) == 0);

    std::vector<std::string> variant_keys;
    get_variant_key_names(variant_code, variant_keys);

    Ref<GLShader> shader;
    if (category == ShaderCategory::GRAPHICS) {
        std::string mixed_vs = shader_source_inject(vs_src, variant_keys);
        std::string mixed_ps = shader_source_inject(ps_src, variant_keys);
        shader = GLShader::compile_graphics(mixed_vs, mixed_ps);
    } else {
        std::string mixed_cs = shader_source_inject(vs_src, variant_keys);
        shader = GLShader::compile_compute(mixed_cs);
    }

    GLShaderIntrospector introspector{shader.get()};

    /*
    纹理单元由着色器中的layout(binding = N)决定（N就是采样器uniform的初值），这里校验它符合引擎约定：
    - 引擎纹理必须使用引擎表中的单元和类型
    - 材质纹理必须分配在引擎保留区之后，且同一个纹理在所有变体中的单元必须一致，否则切换变体时纹理就绑错了
    一个纹理在某个变体中没有被使用时会残留旧的绑定，不影响渲染，只有出现在变体中的纹理才需要检查
    */
    std::unordered_map<uint32_t, std::string> texture_unit_owner; // 单元 -> 纹理名，用于检测单元冲突
    for (auto &&[name, type] : introspector.get_texture_info()) {
        // 采样器数组当前不支持，报错
        if (name.size() > 3 && name.ends_with("[0]")) {
            throw RuntimeError(std::format("纹理\"{}\"是采样器数组，暂不支持",
                                           std::string_view(name).substr(0, name.size() - 3)));
        }
        const std::optional<uint32_t> unit = shader->query_texture_unit(name);
        if (!unit.has_value()) {
            throw RuntimeError(std::format("无法查询纹理\"{}\"的单元", name));
        }

        if (const EngineTextureDecl *engine_texture = find_engine_texture(name); engine_texture != nullptr) {
            const uint32_t expected_unit = engine_texture_unit(engine_texture->slot);
            if (*unit != expected_unit) {
                throw RuntimeError(std::format("引擎纹理\"{}\"必须使用单元{}，即layout(binding = {})，当前为{}", name,
                                               expected_unit, expected_unit, *unit));
            }
            if (type != engine_texture->type) {
                throw RuntimeError(std::format("引擎纹理\"{}\"在着色器中的类型与引擎不符", name));
            }
        } else {
            if (*unit < MATERIAL_TEXTURE_UNIT_BEGIN) {
                throw RuntimeError(std::format("材质纹理\"{}\"被分配到了单元{}，单元0..{}为引擎纹理保留，"
                                               "材质纹理请用layout指定一个不小于{}的单元",
                                               name, *unit, MATERIAL_TEXTURE_UNIT_BEGIN - 1,
                                               MATERIAL_TEXTURE_UNIT_BEGIN));
            }

            auto iter = texture_units.find(name);
            if (iter == texture_units.end()) {
                throw RuntimeError(std::format("材质纹理\"{}\"在着色器中使用，却没有通过#texture声明默认纹理", name));
            }
            TextureParameterInfo &info = iter->second;
            if (info.unit.has_value() && *info.unit != *unit) {
                throw RuntimeError(std::format("纹理\"{}\"在单元{}和{}上都出现过，同一个纹理在不同变体中的单元必须一致，"
                                               "layout不能受变体宏的控制",
                                               name, *info.unit, *unit));
            }
            if (info.type != TextureType::UNKNOWN && info.type != type) {
                throw RuntimeError(std::format("纹理\"{}\"在不同的变体中定义为了不同的类型", name));
            }
            info.unit = unit;
            info.type = type;
        }

        if (auto [owner, inserted] = texture_unit_owner.emplace(*unit, name); !inserted) {
            throw RuntimeError(std::format("纹理\"{}\"和\"{}\"使用了同一个单元{}", owner->second, name, *unit));
        }
    }

    // 图像单元同理
    std::unordered_map<uint32_t, std::string> image_unit_owner; // 单元 -> 图像名，用于检测单元冲突
    for (auto &&[name, type] : introspector.get_image_info()) {
        // 图像数组当前不支持，报错
        if (name.size() > 3 && name.ends_with("[0]")) {
            throw RuntimeError(std::format("图像\"{}\"是数组，暂不支持",
                                           std::string_view(name).substr(0, name.size() - 3)));
        }
        auto iter = image_units.find(name);
        if (iter == image_units.end()) {
            // 图像没有通过#image指令声明，却在着色器中使用
            throw RuntimeError(std::format("图像\"{}\"未通过#image指令声明，却在着色器中使用", name));
        }
        const std::optional<uint32_t> unit = shader->query_texture_unit(name);
        if (!unit.has_value()) {
            throw RuntimeError(std::format("无法查询图像\"{}\"的单元", name));
        }
        if (*unit < MATERIAL_IMAGE_UNIT_BEGIN) {
            throw RuntimeError(std::format("图像\"{}\"被分配到了单元{}，单元0..{}为引擎图像保留，"
                                           "其他图像请用layout指定一个不小于{}的单元",
                                           name, *unit, MATERIAL_IMAGE_UNIT_BEGIN - 1, MATERIAL_IMAGE_UNIT_BEGIN));
        }

        ImageParameterInfo &info = iter->second;
        if (info.unit.has_value() && *info.unit != *unit) {
            throw RuntimeError(std::format("图像\"{}\"在单元{}和{}上都出现过，同一个图像在不同变体中的单元必须一致，"
                                           "layout不能受变体宏的控制",
                                           name, *info.unit, *unit));
        }
        if (info.type != TextureType::UNKNOWN && info.type != type) {
            throw RuntimeError(std::format("图像\"{}\"在不同的变体中定义为了不同的类型", name));
        }
        info.unit = unit;
        info.type = type;

        if (auto [owner, inserted] = image_unit_owner.emplace(*unit, name); !inserted) {
            throw RuntimeError(std::format("图像\"{}\"和\"{}\"使用了同一个单元{}", owner->second, name, *unit));
        }
    }

    shaders.emplace(variant_code, shader);

    return shader;
}

} // namespace Goonya
