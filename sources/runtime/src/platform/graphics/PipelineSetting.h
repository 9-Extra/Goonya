#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <utility>

namespace Goonya {

enum class CullFaceMode : uint8_t {
    BACK = 0,
    FRONT,
    FRONT_AND_BACK,
    DISABLE,

    MAX_
};

enum class DepthTestMode : uint8_t {
    LESS = 0,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    NEVER,
    ALWAYS,
    DISABLE,

    MAX_
};

enum class BlendOp : uint8_t {
    ADD,
    SUB,
    REV_SUB,
    MIN,
    MAX,

    MAX_
};

enum class BlendFactor : uint8_t {
    ZERO,
    ONE,

    SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_SRC_COLOR,
    ONE_MINUS_DST_COLOR,

    SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    ONE_MINUS_DST_ALPHA,

    MAX_
};

struct PipelineSetting {
    DepthTestMode depth_test : 4 = DepthTestMode::LESS;
    CullFaceMode cull_mode : 3 = CullFaceMode::BACK;

    bool is_blend_enable : 1 = false;
    BlendOp blendop_color : 4 = BlendOp::ADD;
    BlendOp blendop_alpha : 4 = BlendOp::ADD;
    BlendFactor src_color_factor : 4 = BlendFactor::SRC_ALPHA;
    BlendFactor dst_color_factor : 4 = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendFactor src_alpha_factor : 4 = BlendFactor::ONE;
    BlendFactor dst_alpha_factor : 4 = BlendFactor::ZERO;
};

/*
传统的Alpha混合，输出颜色与缓冲区颜色以alpha线性插值，alpha值直接输出
is_blend_enable = true;
blendop_color = BlendOp::ADD;
blendop_alpha = BlendOp::ADD;
src_color_factor = BlendFactor::SRC_ALPHA;
dst_color_factor = BlendFactor::ONE_MINUS_SRC_ALPHA;
src_alpha_factor = BlendFactor::ONE;
dst_alpha_factor = BlendFactor::ZERO;

如果alpha值也需要进行混合，则新alpha = src_alpha + (1 - src_alpha) * old_alpha
dst_alpha_factor = BlendFactor::ONE_MINUS_SRC_ALPHA;
可以模拟多层有色玻璃的效果，还需要从远到近渲染
*/

// --------------------------------序列化----------------------------------
/**
 * @brief PipelineSetting中的条目保存在Material中的parameters中的类型
 */
using PipelineSettingParamType = int32_t;

class PipelineSettingSetter {
    struct Entry {
        std::string_view key;
        void (*setter)(PipelineSetting &, PipelineSettingParamType);
        PipelineSettingParamType (*getter)(const PipelineSetting &);
    };

#define DEF_ENTRY_ENUM(name, field_name)                                                                               \
    {.key = (name),                                                                                                    \
     .setter =                                                                                                         \
         [](PipelineSetting &s, PipelineSettingParamType v) {                                                          \
             using EnumType = decltype(s.field_name);                                                                  \
             assert(v <= std::to_underlying(EnumType::MAX_));                                                          \
             s.field_name = EnumType(v);                                                                               \
         },                                                                                                            \
     .getter = [](const PipelineSetting &s) { return PipelineSettingParamType(s.field_name); }}

#define DEF_ENTRY_BOOL(name, field_name)                                                                               \
    {.key = (name),                                                                                                    \
     .setter = [](PipelineSetting &s, PipelineSettingParamType v) { s.field_name = bool(v); },                         \
     .getter = [](const PipelineSetting &s) { return PipelineSettingParamType(s.field_name); }}

    static constexpr Entry ENTRY_TABLE[] = {
        DEF_ENTRY_ENUM("_depth_test", depth_test),
        DEF_ENTRY_ENUM("_cull_mode", cull_mode),
        DEF_ENTRY_BOOL("_is_blend_enable", is_blend_enable),
        DEF_ENTRY_ENUM("_blendop_color", blendop_color),
        DEF_ENTRY_ENUM("_blendop_alpha", blendop_alpha),
        DEF_ENTRY_ENUM("_src_color_factor", src_color_factor),
        DEF_ENTRY_ENUM("_dst_color_factor", dst_color_factor),
        DEF_ENTRY_ENUM("_src_alpha_factor", src_alpha_factor),
        DEF_ENTRY_ENUM("_dst_alpha_factor", dst_alpha_factor),
        DEF_ENTRY_ENUM("_dst_alpha_factor", dst_alpha_factor),
    };

#undef DEF_ENTRY_ENUM
#undef DEF_ENTRY_BOOL

public:
    PipelineSettingSetter() = delete;

    static void set_pipeline_setting(std::string_view key, uint8_t v, PipelineSetting &setting) {
        for (const auto &entry : ENTRY_TABLE) {
            if (entry.key == key) {
                entry.setter(setting, v);
            }
        }
    }

    static void replace_pipeline_setting(std::string_view key, const PipelineSetting &reference,
                                         PipelineSetting &setting) {
        for (const auto &entry : ENTRY_TABLE) {
            if (entry.key == key) {
                entry.setter(setting, entry.getter(reference));
                return;
            }
        }
    }

    static bool is_pipeline_setting(std::string_view key) {
        return std::ranges::any_of(ENTRY_TABLE, [key](auto &&t) { return t.key == key; });
    }
};
}; // namespace Goonya
