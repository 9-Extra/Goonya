#pragma once

#include "../Texture.h"
#include "platform/graphics/opengl/GLBasic.h"
#include <cassert>
#include <cstdint>
#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLTexture: public Texture{
public:
    ~GLTexture(){
        glDeleteTextures(1, &id);
    }
    void bind(uint32_t binding) const noexcept{
        glBindTextureUnit(binding, id);
    }

    GLuint get_id() const noexcept{
        return id;
    }

    void set_filter_mode(TextureFilterMode filter_mode) noexcept;
    void set_warp_mode(TextureWarpMode warp_mode) noexcept;
    void generate_mipmaps() noexcept{
        glGenerateTextureMipmap(id);
    }
protected:
    friend class OpenGLGraphicsAPI;
    GLTexture(TextureType type, std::tuple<uint32_t, uint32_t, uint32_t> shape);
    GLuint id;
};

}
}