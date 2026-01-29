#include "UberShader.h"

#include "core/RefCount.h"
#include "core/log/Log.h"
#include "platform/graphics/opengl/GLShader.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GoonyaException.h"

#include <cstdint>
#include <limits>
#include <ranges>

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
    }
    ss << src.substr(0, injection_point);

    ss << "//------Combined Definitions---------: \n";

    for (const std::string &key : variant_key) {
        ss << std::format("#define {}\n", key);
    }

    ss << "//------Combined Definition End------: \n";

    ss << src.substr(injection_point + PATTERN.size());

    return ss.str();
}

UberShader::UberShader(UberShaderDesc &&desc) {
    this->vs_src = std::move(desc.vs_src);
    this->ps_src = std::move(desc.ps_src);
    this->pipeline_setting = desc.pipeline_setting;
    this->local_variant_key_collect = LocalVariantKeyCollect(std::move(desc.local_variant_keys));
    this->effective_global_key_mask = GLOBAL_VARIANT_KEY.get_shader_global_key_mask(desc.global_variant_keys);

    // 不使用Opengl默认的纹理单元绑定，而是手动分配纹理单元
    for (auto &&[i, default_texture] : std::views::enumerate(std::move(desc.textures))) {
        auto &&[name, texture_res] = std::move(default_texture);
        // 纹理单元从0开始编号，纹理类型信息在反射具体的变体时再获取
        texture_units.emplace(name, TextureParameterInfo{TextureType::UNKNOWN, (uint32_t)i});
        textures.emplace(i, std::move(texture_res));
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
                throw RuntimeError(std::format("参数\"{}\"在.meta中定义为了不同的类型", name));
            }
            field.type_and_default_value = iter->second;
        } else {
            LOG_INFO("参数\"{}\"在.meta中未定义，将使用默认值", name);
        }
    }
}

Ref<GLShader> UberShader::query_variant(VariantCodeSet variant_code) {
    if (auto iter = shaders.find(variant_code); iter != shaders.end()) {
        return iter->second;
    }

    // 获取global_code时，记得先进行effective_global_key_mask掩码操作
    GN_ASSERT(((!effective_global_key_mask) & variant_code.global_code) == 0);

    std::vector<std::string> variant_keys;
    get_variant_key_names(variant_code, variant_keys);
    std::string mixed_vs = shader_source_inject(vs_src, variant_keys);
    std::string mixed_ps = shader_source_inject(ps_src, variant_keys);

    Ref<GLShader> shader = create_ref<GLShader>(mixed_vs, mixed_ps);

    // 手动覆盖纹理单元绑定。这是因为不同变体的纹理单元绑定可能不同，需要手动保证每个变体的纹理单元绑定是一致的
    GLShaderIntrospector introspector{shader.get()};
    std::unordered_map<std::string, TextureType> texture_info = introspector.get_texture_info();
    for (auto &&[name, type] : texture_info) {
        if (auto iter = texture_units.find(name); iter != texture_units.end()) {
            shader->set_texture_binding(name, iter->second.unit);
            if (iter->second.type == TextureType::UNKNOWN) {
                iter->second.type = type;
            } else {
                if (iter->second.type != type) {
                    throw RuntimeError(std::format("纹理\"{}\"在不同的变体中定义为了不同的类型", name));
                }
            }
        } else {
            // 纹理没有在.meta中定义，却在着色器中使用
            throw RuntimeError(std::format("纹理\"{}\"在.meta中未定义，却在着色器中使用", name));
        }
    }

    shaders.emplace(variant_code, shader);

    return shader;
}

} // namespace Goonya
