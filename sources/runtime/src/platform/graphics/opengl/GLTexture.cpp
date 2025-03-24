#include "GLTexture.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "runtime/GoonyaException.h"
#include <FreeImage.h>
#include <cassert>
#include <cstddef>

namespace Goonya {
namespace Graphics {

inline GLsizei max_mipmap_level(size_t width) { return std::log2(width) + 1; }
inline GLsizei max_mipmap_level(size_t width, size_t height) { return std::log2(std::max(width, height)) + 1; }
inline GLsizei max_mipmap_level(size_t width, size_t height, size_t depth) {
    return std::log2(std::max(std::max(width, height), depth)) + 1;
}

static GLenum texture_format_to_gl_format(TextureStorageFormat format) {
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

GLTexture::GLTexture(const TextureCreateDesc &desc) : Texture(desc) {
    const auto &[width, height, depth] = shape;
    const GLenum gl_format = texture_format_to_gl_format(format);

    if (gl_format == 0) {
        throw RuntimeError("不能创建格式为UNKNOWN纹理");
    }

    switch (type) {

    case TextureType::UNKNOWN:
        throw RuntimeError("不能创建类型为UNKNOWN纹理");
    case TextureType::TEXTURE_1D: {
        assert(width != 0 && height == 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_1D, 1, &id);
        glTextureStorage1D(id, max_mipmap_level(width), gl_format, width);
        break;
    }
    case TextureType::TEXTURE_1D_ARRYA: {
        assert(false);
        break;
    }
    case TextureType::TEXTURE_2D: {
        assert(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), gl_format, width, height);
        break;
    }
    case TextureType::TEXTURE_2D_ARRYA: {
        assert(false);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP: {
        assert(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), gl_format, width, height);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP_ARRYA: {
        assert(false);
        break;
    }
    case TextureType::TEXTURE_3D: {
        assert(width != 0 && height != 0 && depth != 0);
        glCreateTextures(GL_TEXTURE_3D, 1, &id);
        glTextureStorage3D(id, max_mipmap_level(width, height, depth), gl_format, width, height, depth);
        break;
    }
    }
    opengl_check_error();
}

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
    opengl_debug_check_error();
}

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
    opengl_debug_check_error();
}
void GLTexture::write_image(FIBITMAP *pImage, uint32_t mipmap_level, uint32_t xoffset, uint32_t yoffset,
                            uint32_t zoffset) {
    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    GLenum source_type = 0, source_format = 0;
    switch (FreeImage_GetImageType(pImage)) {
    case FIT_UINT16: {
        source_type = GL_UNSIGNED_SHORT;
        source_format = GL_RED;
        break;
    }
    case FIT_INT16: {
        source_type = GL_SHORT;
        source_format = GL_RED;
        break;
    }
    case FIT_UINT32: {
        source_type = GL_UNSIGNED_INT;
        source_format = GL_RED;
        break;
    }
    case FIT_INT32: {
        source_type = GL_INT;
        source_format = GL_RED;
        break;
    }
    case FIT_FLOAT: {
        source_type = GL_FLOAT;
        source_format = GL_RED;
        break;
    }
    case FIT_RGB16: {
        source_type = GL_UNSIGNED_SHORT;
        source_format = GL_RGB;
        break;
    }
    case FIT_RGBA16: {
        source_type = GL_UNSIGNED_SHORT;
        source_format = GL_RGBA;
        break;
    }
    case FIT_RGBF: {
        source_type = GL_FLOAT;
        source_format = GL_RGB;
        break;
    }
    case FIT_RGBAF: {
        source_type = GL_FLOAT;
        source_format = GL_RGBA;
        break;
    }
    case FIT_BITMAP: {
        unsigned int bpp = FreeImage_GetBPP(pImage);
        source_type = GL_UNSIGNED_BYTE;
        if (bpp == 24) {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            source_format = GL_BGR;
#else
            source_format = GL_RBG;
#endif
        } else if (bpp == 32) {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            source_format = GL_BGRA;
#else
            source_format = GL_RBGA;
#endif
        } else {
            throw RuntimeError("不支持的图像像素格式");
        }
        break;
    }
    default: {
        throw RuntimeError("不支持的图像像素格式");
    }
    }
    if (type == TextureType::TEXTURE_2D || type == TextureType::TEXTURE_1D_ARRYA) {
        glTextureSubImage2D(id, mipmap_level, xoffset, yoffset, nWidth, nHeight, source_format, source_type,
                            FreeImage_GetBits(pImage));
    } else if (type == TextureType::TEXTURE_2D_ARRYA || type == TextureType::TEXTURE_CUBEMAP ||
               type == TextureType::TEXTURE_CUBEMAP_ARRYA || type == TextureType::TEXTURE_3D) {
        // CubeMap可以使用3D纹理的加载函数进行加载，使用zoffset参数制定加载的图像的方向
        glTextureSubImage3D(id, mipmap_level, xoffset, yoffset, zoffset, nWidth, nHeight, 1, source_format, source_type,
                            FreeImage_GetBits(pImage));
    } else {
        // todo
        assert(false);
    }
    opengl_check_error();
}
} // namespace Graphics
} // namespace Goonya