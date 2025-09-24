#include "Texture.h"
#include "platform/graphics/Graphics.h"

namespace Goonya::Graphics {

static Graphics::TextureStorageFormat get_proper_storage_type(const stb::Image &image) noexcept {
    Graphics::TextureStorageFormat storage_type = Graphics::TextureStorageFormat::UNKNOWN;
    switch (image.get_channel()) {
    case 1: {
        storage_type = image.is_float() ? Graphics::TextureStorageFormat::R_f32 : Graphics::TextureStorageFormat::R_f8;
        break;
    }
    case 2:
    case 3: {
        storage_type =
            image.is_float() ? Graphics::TextureStorageFormat::RGB_f32 : Graphics::TextureStorageFormat::RGB_f8;
        break;
    }
    case 4: {
        storage_type =
            image.is_float() ? Graphics::TextureStorageFormat::RGBA_f32 : Graphics::TextureStorageFormat::RGBA_f8;
        break;
    }
    }
    return storage_type;
}

Ref<Graphics::Texture> Texture2DContainer::load(const Graphics::Texture2DDesc &desc) const {
    stb::Image image = stb::Image::load(desc.path, desc.is_color);
    if (!image) {
        throw RuntimeError(std::format("图像{}加载失败", desc.path.string()));
    }

    uint32_t width = image.get_width();
    uint32_t height = image.get_height();

    Graphics::TextureStorageFormat storage_type = get_proper_storage_type(image);

    if (storage_type == Graphics::TextureStorageFormat::UNKNOWN) {
        throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", desc.path.string()));
    }
    Graphics::TextureCreateDesc texture_desc{Graphics::TextureType::TEXTURE_2D, storage_type, {width, height, 0}};

    Ref<Graphics::Texture> texture = Graphics::graphics_api->create_texture(texture_desc);
    texture->set_filter_mode(desc.filter_mode);
    texture->set_warp_mode(desc.warp_mode);

    texture->import_image(image, 0);
    texture->generate_mipmaps();

    return texture;
};

Ref<Graphics::Texture> TextureCubeMapContainer::load(const Graphics::TextureCubeMapDesc &desc) const {
    using Graphics::TextureStorageFormat;
    // 使用第一张图像的宽高信息分配纹理空间
    stb::Image image = stb::Image::load(desc.path[0], desc.is_color);
    if (!image) {
        throw RuntimeError(std::format("图像{}加载失败", desc.path[0].string()));
    }

    int width = image.get_width();
    int height = image.get_height();

    TextureStorageFormat storage_type = get_proper_storage_type(image);
    if (storage_type == TextureStorageFormat::UNKNOWN) {
        throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", desc.path[0].string()));
    }
    Graphics::TextureCreateDesc texture_desc{
        Graphics::TextureType::TEXTURE_CUBEMAP, storage_type, {(uint32_t)width, (uint32_t)height, 0}};
    Ref<Graphics::Texture> texture = Graphics::graphics_api->create_texture(texture_desc);
    texture->set_filter_mode(desc.filter_mode);

    texture->import_image(image, 0, 0, 0, 0);

    // 加载其余方向上的图像
    for (unsigned int i = 1; i < desc.path.size(); i++) {
        stb::Image image = stb::Image::load(desc.path[i], desc.is_color);
        if (!image) {
            throw RuntimeError(std::format("图像{}加载失败", desc.path[i].string()));
        }
        if (width != image.get_width() || height != image.get_height()) {
            throw RuntimeError(std::format("CubeMap{}的大小不一致", desc.path[i].string()));
        }
        texture->import_image(image, 0, 0, 0, i);
    }
    texture->generate_mipmaps();

    return texture;
};

} // namespace Goonya::Graphics