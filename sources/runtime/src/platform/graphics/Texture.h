#pragma once

#include "core/intrusive_ptr.h"
#include <cstdint>

#include <string>
#include <tuple>
#include <array>

namespace Goonya {
namespace Graphics {

// ============纹理================
enum class TextureType {
    UNKNOWN = 0,

    TEXTURE_1D,
    TEXTURE_1D_ARRYA,
    TEXTURE_2D,
    TEXTURE_2D_ARRYA,
    TEXTURE_CUBEMAP,
    TEXTURE_CUBEMAP_ARRYA,
    TEXTURE_3D,
};

enum class TextureWarpMode{
    REPEAT,
    ClAMP,
    MIRROR
};

enum class TextureFilterMode{
    NEAREST,
    BILINEAR,
    TRILINEAR
};

struct Texture2DDesc {
    std::string path;
    bool is_srgb = false;         // 是否需要转换到线性空间
    bool is_uv_left_down = false; // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};

struct TextureCubeMapDesc {
    std::array<std::string, 6> path; // px, nx, py, ny, pz, nz
    bool is_srgb = false;            // 是否需要转换到线性空间
    bool is_uv_left_down = false;    // UV坐标系是否以左下角为原点
    Graphics::TextureFilterMode filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    Graphics::TextureWarpMode warp_mode = Graphics::TextureWarpMode::REPEAT;
};


class Texture : public intrusive_ptr_base<Texture> {
public:
    virtual ~Texture() = default;
    virtual void bind(uint32_t binding) const noexcept = 0;

    TextureType get_type() const noexcept { return type; }

    std::tuple<uint32_t, uint32_t, uint32_t> get_shape() const noexcept { return shape; }

protected:
    Texture(TextureType type, std::tuple<uint32_t, uint32_t, uint32_t> shape) : type(type), shape(shape) {}

    TextureType type;
    std::tuple<uint32_t, uint32_t, uint32_t> shape; // width, height, depth, 如果对于维度不存在则为0
};

} // namespace Graphics
} // namespace Goonya