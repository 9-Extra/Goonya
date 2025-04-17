#pragma once

#include "core/intrusive_ptr.h"
#include <cstdint>

#include <tuple>

struct FIBITMAP;

namespace Goonya::Graphics {

// ============纹理================
enum class TextureType {
    UNKNOWN = 0,

    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBEMAP,
    TEXTURE_CUBEMAP_ARRAY,
    TEXTURE_3D,
};

enum class TextureStorageFormat {
    UNKNOWN = 0,

    RGBA_f32,
    RGBA_i32,
    RGBA_u32,
    RGBA_f16,
    RGBA_i16,
    RGBA_u16,
    RGBA_f8, // 8位无符号归一化为浮点数 [0, 255] -> [0.0f, 1.0f]
    RGBA_i8,
    RGBA_u8,
    RGB_f32,
    RGB_i32,
    RGB_u32,
    RGB_f16,
    RGB_i16,
    RGB_u16,
    RGB_f8,
    RGB_i8,
    RGB_u8,
    R_f32,
    R_i32,
    R_u32,
    R_f16,
    R_i16,
    R_u16,
    R_f8,
    R_i8,
    R_u8,
};

enum class TextureWarpMode { REPEAT, ClAMP, MIRROR };

enum class TextureFilterMode { NEAREST, BILINEAR, TRILINEAR };

struct TextureCreateDesc {
    TextureType type;
    TextureStorageFormat format;
    std::tuple<uint32_t, uint32_t, uint32_t> shape;
};

class Texture : public intrusive_ptr_base<Texture> {
public:
    virtual ~Texture() = default;
    virtual void bind(uint32_t binding) const noexcept = 0;
    virtual void set_filter_mode(TextureFilterMode filter_mode) noexcept = 0;
    virtual void set_warp_mode(TextureWarpMode warp_mode) noexcept = 0;
    virtual void generate_mipmaps() noexcept = 0;

    virtual void import_image(FIBITMAP *image, uint32_t mipmap_level = 0, uint32_t xoffset = 0, uint32_t yoffset = 0,
                              uint32_t zoffset = 0) = 0;
    virtual FIBITMAP *export_image(uint32_t mipmap_level = 0, uint32_t zoffset = 0) const = 0;

    TextureType get_type() const noexcept { return type; }

    std::tuple<uint32_t, uint32_t, uint32_t> get_shape() const noexcept { return shape; }

protected:
    explicit Texture(const TextureCreateDesc &desc) : type(desc.type), format(desc.format), shape(desc.shape) {}

    TextureType type;
    TextureStorageFormat format;
    std::tuple<uint32_t, uint32_t, uint32_t> shape; // width, height, depth, 如果对于维度不存在则为0
};

} // namespace Goonya::Graphics
