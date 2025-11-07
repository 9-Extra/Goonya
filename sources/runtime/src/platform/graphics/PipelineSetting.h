#pragma once

#include <cstdint>

namespace Goonya::Graphics {

enum class CullFaceMode : uint8_t {
    BACK = 0,
    FRONT,
    FRONT_AND_BACK,
    DISABLE,
};

enum class DepthTestMode : uint8_t {
    LESS = 0,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    NEVER,
    ALWAYS,
    DISABLE,
};

enum class BlendOp : uint8_t {
    ADD,
    SUB,
    REV_SUB,
    MIN,
    MAX,
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
    ONE_MINUS_DST_ALPHA
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

}; // namespace Goonya::Graphics