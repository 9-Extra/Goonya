#include "GLTexture.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "runtime/GoonyaException.h"
#include <cassert>
#include <cstddef>

namespace Goonya {
namespace Graphics {

inline GLsizei max_mipmap_level(size_t width){
    return std::log2(width) + 1;
}
inline GLsizei max_mipmap_level(size_t width, size_t height){
    return std::log2(std::max(width, height)) + 1;
}
inline GLsizei max_mipmap_level(size_t width, size_t height, size_t depth){
    return std::log2(std::max(std::max(width, height), depth)) + 1;
}

GLTexture::GLTexture(TextureType type, std::tuple<uint32_t, uint32_t, uint32_t> shape) : Texture(type, shape) {
    const auto &[width, height, depth] = shape;

    switch (type) {

    case TextureType::UNKNOWN:
        throw RuntimeError("不能创建类型为UNKNOWN纹理");
    case TextureType::TEXTURE_1D: {
        assert(width != 0 && height == 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_1D, 1, &id);
        glTextureStorage1D(id, max_mipmap_level(width), GL_RGB32F, width);
        break;
    }
    case TextureType::TEXTURE_1D_ARRYA:{
        assert(false);
        break;
    }
    case TextureType::TEXTURE_2D:{
        assert(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), GL_RGB32F, width, height);
        break;
    }
    case TextureType::TEXTURE_2D_ARRYA:{
        assert(false);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP:{
        assert(width != 0 && height != 0 && depth == 0);
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, max_mipmap_level(width, height), GL_RGB32F, width, height);
        break;
    }
    case TextureType::TEXTURE_CUBEMAP_ARRYA:{
        assert(false);
        break;
    }
    case TextureType::TEXTURE_3D:{
        assert(width != 0 && height != 0 && depth != 0);
        glCreateTextures(GL_TEXTURE_3D, 1, &id);
        glTextureStorage3D(id, max_mipmap_level(width, height, depth), GL_RGB32F, width, height, depth);
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
} // namespace Graphics
} // namespace Goonya