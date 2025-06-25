#pragma once

#include "../Texture.h"
#include "platform/graphics/opengl/GLBasic.h"

#include <cassert>
#include <cstdint>
#include <glad/glad.h>

namespace Goonya::Graphics {

class GLTexture final : public Texture {
protected:
    GLuint id = 0;
public:
    explicit GLTexture(const TextureCreateDesc &desc);
    ~GLTexture() override { glDeleteTextures(1, &id); }
    void bind(uint32_t binding) const noexcept override { glBindTextureUnit(binding, id); }

    GLuint get_id() const noexcept { return id; }

    void set_filter_mode(TextureFilterMode filter_mode) noexcept override;
    void set_warp_mode(TextureWarpMode warp_mode) noexcept override;
    void generate_mipmaps() noexcept override { glGenerateTextureMipmap(id); }

    void import_image(const stb::Image& image, uint32_t mipmap_level = 0, uint32_t xoffset = 0, uint32_t yoffset = 0,
                              uint32_t zoffset = 0) override;
    stb::Image export_image(uint32_t mipmap_level = 0, uint32_t zoffset = 0) const override;

};

} // namespace Goonya::Graphics
