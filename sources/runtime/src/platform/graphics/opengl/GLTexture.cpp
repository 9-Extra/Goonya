#include "GLTexture.h"

#include "runtime/GAssert.h"
#include "runtime/GoonyaException.h"

#include <cmath>
#include <cstdint>

namespace Goonya {

inline GLsizei max_mipmap_level(size_t width) { return static_cast<GLsizei>(std::log2(width)) + 1; }
inline GLsizei max_mipmap_level(size_t width, size_t height) {
    return static_cast<GLsizei>(std::log2(std::max(width, height))) + 1;
}
inline GLsizei max_mipmap_level(size_t width, size_t height, size_t depth) {
    return static_cast<GLsizei>(std::log2(std::max(std::max(width, height), depth))) + 1;
}

GLTexture::GLTexture(const TextureCreateDesc &desc) : GLTexture(desc.type, desc.format, desc.shape) {}

GLTexture::GLTexture(TextureType type, TextureStorageFormat format, std::tuple<uint32_t, uint32_t, uint32_t> shape)
    : type(type), format(format), shape(shape) {
    const auto &[width, height, depth] = this->shape;
    const GLenum gl_format = texture_format_to_gl_format(this->format);

    if (gl_format == 0) {
        throw RuntimeError("不能创建格式为UNKNOWN纹理");
    }

    switch (this->type) {

    case TextureType::UNKNOWN:
        throw RuntimeError("不能创建类型为UNKNOWN纹理");
    case TextureType::TEXTURE_1D: {
        GN_ASSERT(width != 0 && height == 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_1D, 1, &id);
        glTextureStorage1D(id, max_mipmap_level(width), gl_format, width);
        break;
    }
    case TextureType::TEXTURE_1D_ARRAY: {
        GN_ASSERT(false);
        break;
    }
    case TextureType::TEXTURE_2D: {
        GN_ASSERT(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), gl_format, width, height);
        break;
    }
    case TextureType::TEXTURE_2D_ARRAY: {
        GN_ASSERT(width != 0 && height != 0 && depth != 0);
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
        glTextureStorage3D(id, max_mipmap_level(width, height), gl_format, width, height, depth);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP: {
        GN_ASSERT(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), gl_format, width, height);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP_ARRAY: {
        GN_ASSERT(false);
        break;
    }
    case TextureType::TEXTURE_3D: {
        GN_ASSERT(width != 0 && height != 0 && depth != 0);
        glCreateTextures(GL_TEXTURE_3D, 1, &id);
        glTextureStorage3D(id, max_mipmap_level(width, height, depth), gl_format, width, height, depth);
        break;
    }
    }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void GLTexture::set_warp_mode(TextureWarpMode warp_mode) noexcept {
    GLenum gl_warp_mode;
    if (warp_mode == TextureWarpMode::REPEAT) {
        gl_warp_mode = GL_REPEAT;
    } else if (warp_mode == TextureWarpMode::ClAMP) {
        gl_warp_mode = GL_CLAMP_TO_EDGE;
    } else if (warp_mode == TextureWarpMode::MIRROR) {
        gl_warp_mode = GL_MIRRORED_REPEAT;
    } else {
        std::unreachable();
    }
    // 无论实际上有多少维，反正都设置
    glTextureParameteri(id, GL_TEXTURE_WRAP_R, gl_warp_mode);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, gl_warp_mode);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, gl_warp_mode);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void GLTexture::set_filter_mode(TextureFilterMode filter_mode) noexcept {
    GLenum min_filter, mag_filter;
    if (filter_mode == TextureFilterMode::NEAREST) {
        min_filter = GL_NEAREST;
        mag_filter = GL_NEAREST;
    } else if (filter_mode == TextureFilterMode::BILINEAR) {
        min_filter = GL_LINEAR;
        mag_filter = GL_LINEAR;
    } else if (filter_mode == TextureFilterMode::TRILINEAR) {
        min_filter = GL_LINEAR_MIPMAP_LINEAR;
        mag_filter = GL_LINEAR;
    } else {
        std::unreachable();
    }
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, min_filter);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void GLTexture::import_image(const stb::Image &image, uint32_t mipmap_level, uint32_t xoffset, uint32_t yoffset,
                             uint32_t zoffset) {

    // 对于CUBEMAP纹理，由于OpenGL中CUBEMAP的“别出心裁”的构思设定，为了使得可以正确使用方向采样CUBEMAP纹理，进行一个上下翻转
    // 对于其他纹理，由于Goonya定义的纹理坐标uv的原点为左上角（与DX一致），而OpenGL为左下角，解决方法是对纹理进行翻转
    // 结果就是，所有的类型纹理都需要翻转一下

    // 而在加载纹理时glTextureSubImage2D将内存中的第一行数据视为纹理的最底部行（左下角），而stb::Image中第一行数据为图像数据的最顶部行
    // 这意味这stb加载的图像对与OpenGL来说本来就是翻转的
    // 所以结果就是不需要翻转了
    unsigned int width = image.get_width();
    unsigned int height = image.get_height();

    GLenum source_type = image.is_float() ? GL_FLOAT : GL_UNSIGNED_BYTE;
    GLenum source_format = 0;
    switch (image.get_channel()) {
    case 1: {
        source_format = GL_RED;
        break;
    }
    case 2: {
        source_format = GL_RG;
        break;
    }
    case 3: {
        source_format = GL_RGB;
        break;
    }
    case 4: {
        source_format = GL_RGBA;
        break;
    }
    }
    if (source_type == 0 || source_format == 0) {
        throw RuntimeError("不支持的图像像素格式");
    }

    switch (this->type) {
    case TextureType::TEXTURE_1D_ARRAY:
    case TextureType::TEXTURE_2D: {
        glTextureSubImage2D(id, mipmap_level, xoffset, yoffset, width, height, source_format, source_type,
                            image.get_data());
        break;
    }
    case TextureType::TEXTURE_2D_ARRAY:
    case TextureType::TEXTURE_CUBEMAP:
    case TextureType::TEXTURE_CUBEMAP_ARRAY:
    case TextureType::TEXTURE_3D: {
        // CubeMap可以使用3D纹理的加载函数进行加载，使用zoffset参数制定加载的图像的方向
        glTextureSubImage3D(id, mipmap_level, xoffset, yoffset, zoffset, width, height, 1, source_format, source_type,
                            image.get_data());
        break;
    }
    default:
        throw RuntimeError("不支持此类型");
    }
}

stb::Image GLTexture::export_image(uint32_t mipmap_level, uint32_t zoffset) const {
    GLuint level;
    glGetTextureParameterIuiv(id, GL_TEXTURE_MAX_LEVEL, &level);
    if (mipmap_level > level) {
        throw RuntimeError("指定的Level超过上限");
    }

    GLint width, height;
    glGetTextureLevelParameteriv(id, mipmap_level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(id, mipmap_level, GL_TEXTURE_HEIGHT, &height);

    GLenum target_format = 0;
    int channel = 0;
    bool is_float = false;

    switch (this->type) {

    case TextureType::TEXTURE_CUBEMAP:
    case TextureType::TEXTURE_2D:
    case TextureType::TEXTURE_2D_ARRAY:
    case TextureType::TEXTURE_3D: {
        switch (this->format) {
        case TextureStorageFormat::RGBA_f32:
        case TextureStorageFormat::RGBA_i32:
        case TextureStorageFormat::RGBA_u32:
        case TextureStorageFormat::RGBA_f16:
        case TextureStorageFormat::RGBA_i16:
        case TextureStorageFormat::RGBA_u16: {
            target_format = GL_RGBA;
            channel = 4;
            is_float = true;
            break;
        }
        case TextureStorageFormat::RGBA_f8:
        case TextureStorageFormat::RGBA_i8:
        case TextureStorageFormat::RGBA_u8: {
            target_format = GL_RGBA;
            channel = 4;
            is_float = false;
            break;
        }
        case TextureStorageFormat::RGB_f32:
        case TextureStorageFormat::RGB_i32:
        case TextureStorageFormat::RGB_u32:
        case TextureStorageFormat::RGB_f16:
        case TextureStorageFormat::RGB_i16:
        case TextureStorageFormat::RGB_u16: {
            target_format = GL_RGB;
            channel = 3;
            is_float = true;
            break;
        }
        case TextureStorageFormat::RGB_f8:
        case TextureStorageFormat::RGB_i8:
        case TextureStorageFormat::RGB_u8: {
            target_format = GL_RGB;
            channel = 3;
            is_float = false;
            break;
        }
        case TextureStorageFormat::R_f32:
        case TextureStorageFormat::R_i32:
        case TextureStorageFormat::R_u32:
        case TextureStorageFormat::R_f16:
        case TextureStorageFormat::R_i16:
        case TextureStorageFormat::R_u16: {
            target_format = GL_RED;
            channel = 1;
            is_float = true;
            break;
        }
        case TextureStorageFormat::R_f8:
        case TextureStorageFormat::R_i8:
        case TextureStorageFormat::R_u8: {
            target_format = GL_RED;
            channel = 1;
            is_float = false;
            break;
        }
        default: {
            throw RuntimeError("不支持的格式");
        }
        }

        break;
    }

    case TextureType::UNKNOWN:
    case TextureType::TEXTURE_1D:
    case TextureType::TEXTURE_1D_ARRAY:
    case TextureType::TEXTURE_CUBEMAP_ARRAY: {
        throw RuntimeError("不支持");
    }
    }

    stb::Image image = stb::Image::create_empty(width, height, channel, is_float);
    unsigned int buf_size = image.get_size_byte();
    GLenum target_type = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glGetTextureSubImage(id, mipmap_level, 0, 0, zoffset, width, height, 1, target_format, target_type, buf_size,
                         image.get_data());
    return image;
};

GLenum texture_format_to_gl_format(TextureStorageFormat format) {
    switch (format) {
    case TextureStorageFormat::RGBA_f32:
        return GL_RGBA32F;
    case TextureStorageFormat::RGBA_i32:
        return GL_RGBA32I;
    case TextureStorageFormat::RGBA_u32:
        return GL_RGBA32UI;
    case TextureStorageFormat::RGBA_f16:
        return GL_RGBA16;
    case TextureStorageFormat::RGBA_i16:
        return GL_RGBA16I;
    case TextureStorageFormat::RGBA_u16:
        return GL_RGBA16UI;
    case TextureStorageFormat::RGBA_f8:
        return GL_RGBA8;
    case TextureStorageFormat::RGBA_i8:
        return GL_RGBA8I;
    case TextureStorageFormat::RGBA_u8:
        return GL_RGBA8UI;
    case TextureStorageFormat::RGB_f32:
        return GL_RGB32F;
    case TextureStorageFormat::RGB_i32:
        return GL_RGB32I;
    case TextureStorageFormat::RGB_u32:
        return GL_RGB32UI;
    case TextureStorageFormat::RGB_f16:
        return GL_RGB16;
    case TextureStorageFormat::RGB_i16:
        return GL_RGB16I;
    case TextureStorageFormat::RGB_u16:
        return GL_RGB16UI;
    case TextureStorageFormat::RGB_f8:
        return GL_RGB8;
    case TextureStorageFormat::RGB_i8:
        return GL_RGB8I;
    case TextureStorageFormat::RGB_u8:
        return GL_RGB8UI;
    case TextureStorageFormat::R_f32:
        return GL_R32F;
    case TextureStorageFormat::R_i32:
        return GL_R32I;
    case TextureStorageFormat::R_u32:
        return GL_R32UI;
    case TextureStorageFormat::R_f16:
        return GL_R16;
    case TextureStorageFormat::R_i16:
        return GL_R16I;
    case TextureStorageFormat::R_u16:
        return GL_R16UI;
    case TextureStorageFormat::R_f8:
        return GL_R8;
    case TextureStorageFormat::R_i8:
        return GL_R8I;
    case TextureStorageFormat::R_u8:
        return GL_R8UI;
    case TextureStorageFormat::UNKNOWN:
        break;
    }
    return 0;
};
} // namespace Goonya
