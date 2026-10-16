#pragma once

#include "core/cgmath/vector.h"
#include "platform/image/image.h"
#include "resource/Resource.h"

#include <glad/glad.h>

#include <cstdint>

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
    RG_f32,
    RG_i32,
    RG_u32,
    RG_f16,
    RG_i16,
    RG_u16,
    RG_f8,
    RG_i8,
    RG_u8,
    R_f32,
    R_i32,
    R_u32,
    R_f16,
    R_i16,
    R_u16,
    R_f8,
    R_i8,
    R_u8,

    DEPTH_16, // 所有的depth读取时都相当于归一化浮点数，只有DEPTH_f32真正使用浮点存储
    DEPTH_24,
    DEPTH_32,
    DEPTH_f32,

    STENCIL_1,
    STENCIL_4,
    STENCIL_8,
    STENCIL_16,

    DEPTH_24_STENCIL_8,
    DEPTH_f32_STENCIL_8,
};

GLenum texture_format_to_gl_format(TextureStorageFormat format);

enum class TextureWarpMode { REPEAT, CLAMP, MIRROR };

enum class TextureFilterMode { NEAREST, BILINEAR, TRILINEAR };

class GLTexture final : public Resource {
private:
    GLuint id = 0;

    TextureType type;
    TextureStorageFormat format;
    Vector3i shape; // width, height, depth, 如果对于维度不存在则为1
    uint8_t mipmap_levels = 255;

public:
    explicit GLTexture(TextureType type, TextureStorageFormat format, Vector3i shape,
                       uint8_t mipmap_levels = 255);
    ~GLTexture() { glDeleteTextures(1, &id); }
    void bind_texture(uint32_t binding) const noexcept { glBindTextureUnit(binding, id); }
    // gl_access和gl_type从着色器的#image指令获取
    // layered为true时绑定整张纹理（cube map的全部6面、整个数组），此时layer被忽略；为false时绑定layer指定的那一层
    void bind_image(uint32_t binding, uint32_t mipmap_level, bool layered, uint32_t layer, uint32_t gl_access,
                    uint32_t gl_type) const noexcept {
        glBindImageTexture(binding, id, mipmap_level, layered ? GL_TRUE : GL_FALSE, layer, gl_access, gl_type);
    }

    uint32_t get_id() const noexcept { return id; }

    void set_filter_mode(TextureFilterMode filter_mode) noexcept;
    void set_warp_mode(TextureWarpMode warp_mode) noexcept;
    void generate_mipmaps() noexcept // NOLINT
    {
        glGenerateTextureMipmap(id);
    }
    void generate_prefiltered_envmap(); // 会加载预滤波着色器，加载或编译失败时抛出异常

    void import_image(const stb::Image &image, uint32_t mipmap_level = 0, uint32_t xoffset = 0, uint32_t yoffset = 0,
                      uint32_t zoffset = 0);
    stb::Image export_image(uint32_t mipmap_level = 0, uint32_t zoffset = 0) const;
    // 复制纹理，没有的维度尺寸需要为1而不是0
    void copy_image_from(const Ref<GLTexture>& src, uint32_t src_mipmap_level, uint32_t dst_mipmap_level, Vector3i size, Vector3i src_offset = {}, Vector3i dst_offset = {}) noexcept;

    TextureType get_type() const noexcept { return type; }
    TextureStorageFormat get_interal_format() const noexcept { return format; }

    Vector3i get_shape() const noexcept { return shape; }
    uint8_t get_mipmap_levels() const noexcept { return mipmap_levels; }
    // 可被layer指定的层数：cube map为6，1D/2D数组为数组长度，其余类型没有layer的概念为1
    // 返回nullopt表示此类型的layer语义尚未确认（Texture3D）
    int32_t get_layer_count() const noexcept;

    static TextureStorageFormat get_proper_storage_type(const stb::Image &image) noexcept;
};

} // namespace Goonya
