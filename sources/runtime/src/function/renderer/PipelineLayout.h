#pragma once

#include "core/cgmath/cgmath.h"
#include "platform/graphics/opengl/GLTexture.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace Goonya {
static constexpr unsigned int POINTLIGHT_MAX = 8;

struct PointLightData final {
    alignas(16) Vector3f position;
    alignas(16) Vector3f intensity;
};

struct PerFrameData final { // NOLINT：不需要初始化
    Matrix4f view_matrix;
    Matrix4f view_matrix_inv;
    Matrix4f perspective_matrix;
    Matrix4f view_perspective_matrix;
    alignas(16) Vector3f ambient_light;
    alignas(16) Vector3f camera_position;
    alignas(4) float fog_min_distance;
    alignas(4) float fog_density;
    alignas(4) float time;
    alignas(4) Vector2f screen_size;
    alignas(4) uint32_t pointlight_num;
    PointLightData pointlight_list[POINTLIGHT_MAX];
};

constexpr uint32_t PER_FRAME_UNIFORM_BINDING = 0;

struct alignas(256) PerObjectData final {
    Matrix4f model_matrix;
    Matrix4f normal_matrix; // 内存对齐
};

constexpr uint32_t PER_OBJECT_UNIFORM_BINDING = 1;
constexpr uint32_t PER_PASS_UNIFORM_BINDING = 2;
constexpr uint32_t PER_MATERIAL_UNIFORM_BINDING = 3;

constexpr uint32_t BUILDIN_UNIFORM_BINDING_MAX = 8;

/**
 * 引擎纹理
 * 纹理单元0..7为引擎保留，由渲染管线绑定，材质绑定不碰这些单元；
 * 材质自身的纹理必须分配在8及以上，随材质切换而重新绑定。
 * 着色器中的引擎纹理必须用layout(binding = N)声明为表中的单元，名字和类型都要与表一致
 */
enum class EngineTexture : uint32_t {
    BRDF_LUT = 0,
    PREFILTERED_ENVMAP = 1,
    SKYBOX_SPECULAR_TEXTURE = 2,
    CAMERA_DEPTH = 3,
};

struct EngineTextureDecl final {
    std::string_view name;
    EngineTexture slot;
    TextureType type;
};

inline constexpr std::array ENGINE_TEXTURES = {
    EngineTextureDecl{"brdf_lut", EngineTexture::BRDF_LUT, TextureType::TEXTURE_2D},
    EngineTextureDecl{"prefiltered_envmap", EngineTexture::PREFILTERED_ENVMAP, TextureType::TEXTURE_CUBEMAP},
    EngineTextureDecl{"skybox_specular_texture", EngineTexture::SKYBOX_SPECULAR_TEXTURE, TextureType::TEXTURE_CUBEMAP},
    EngineTextureDecl{"camera_depth", EngineTexture::CAMERA_DEPTH, TextureType::TEXTURE_2D},
};

constexpr uint32_t engine_texture_unit(EngineTexture slot) noexcept { return std::to_underlying(slot); }

// 材质纹理/图像必须分配在引擎保留区之后
constexpr uint32_t MATERIAL_TEXTURE_UNIT_BEGIN = 8;
constexpr uint32_t MATERIAL_IMAGE_UNIT_BEGIN = 2; // 图像单元0..1为引擎保留，目前没有固定的引擎图像

constexpr const EngineTextureDecl *find_engine_texture(std::string_view name) noexcept {
    for (const EngineTextureDecl &decl : ENGINE_TEXTURES) {
        if (decl.name == name) {
            return &decl;
        }
    }
    return nullptr;
}
} // namespace Goonya