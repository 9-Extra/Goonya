#pragma once

#include "platform/image/image.h"
#include "resource/Resource.h"

#include <glad/glad.h>

#include <cstdint>
#include <tuple>

namespace Goonya {

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

GLenum texture_format_to_gl_format(TextureStorageFormat format);

class GLTexture final : public Resource {
private:
    GLuint id = 0;

    TextureType type;
    TextureStorageFormat format;
    std::tuple<uint32_t, uint32_t, uint32_t> shape; // width, height, depth, 如果对于维度不存在则为0
public:
    explicit GLTexture(TextureType type, TextureStorageFormat format, std::tuple<uint32_t, uint32_t, uint32_t> shape,
                       uint8_t mipmap_level = 255);
    ~GLTexture() { glDeleteTextures(1, &id); }
    void bind(uint32_t binding) const noexcept { glBindTextureUnit(binding, id); }

    GLuint get_id() const noexcept { return id; }

    void set_filter_mode(TextureFilterMode filter_mode) noexcept;
    void set_warp_mode(TextureWarpMode warp_mode) noexcept;
    void generate_mipmaps() noexcept // NOLINT
    {
        glGenerateTextureMipmap(id);
    }

    void import_image(const stb::Image &image, uint32_t mipmap_level = 0, uint32_t xoffset = 0, uint32_t yoffset = 0,
                      uint32_t zoffset = 0);
    stb::Image export_image(uint32_t mipmap_level = 0, uint32_t zoffset = 0) const;

    TextureType get_type() const noexcept { return type; }

    std::tuple<uint32_t, uint32_t, uint32_t> get_shape() const noexcept { return shape; }

    static TextureStorageFormat get_proper_storage_type(const stb::Image &image) noexcept {
        TextureStorageFormat storage_type = TextureStorageFormat::UNKNOWN;
        switch (image.get_channel()) {
        case 1: {
            storage_type = image.is_float() ? TextureStorageFormat::R_f32 : TextureStorageFormat::R_f8;
            break;
        }
        case 2:
        case 3: {
            storage_type = image.is_float() ? TextureStorageFormat::RGB_f32 : TextureStorageFormat::RGB_f8;
            break;
        }
        case 4: {
            storage_type = image.is_float() ? TextureStorageFormat::RGBA_f32 : TextureStorageFormat::RGBA_f8;
            break;
        }
        }
        return storage_type;
    }
};

} // namespace Goonya
