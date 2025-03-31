#pragma once

#include "../Texture.h"
#include "platform/graphics/opengl/GLBasic.h"
#include <cassert>
#include <cstdint>
#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLTexture : public Texture {
public:
    GLTexture(const TextureCreateDesc& desc);
    ~GLTexture() { glDeleteTextures(1, &id); }
    virtual void bind(uint32_t binding) const noexcept override { glBindTextureUnit(binding, id); }

    GLuint get_id() const noexcept { return id; }

    virtual void set_filter_mode(TextureFilterMode filter_mode) noexcept override;
    virtual void set_warp_mode(TextureWarpMode warp_mode) noexcept override;
    virtual void generate_mipmaps() noexcept override { glGenerateTextureMipmap(id); }

    virtual void import_image(FIBITMAP* image, uint32_t mipmap_level = 0, uint32_t xoffset = 0, uint32_t yoffset = 0, uint32_t zoffset = 0) override;
    virtual FIBITMAP *export_image(uint32_t mipmap_level = 0, uint32_t zoffset = 0) const override;

protected:
    GLuint id;
};

} // namespace Graphics
} // namespace Goonya