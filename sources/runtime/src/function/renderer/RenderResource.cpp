#include "RenderResource.h"
#include "platform/graphics/graphics.h"

#include <FreeImage.h>
#include <glad/glad.h>
#include <nowide/convert.hpp>

namespace Goonya {
namespace Graphics {

RenderReousce resources; // Global

// 利用重载的方式实现对不同路径类型的支持
[[maybe_unused]] static FIBITMAP *freeimage_open_image(const std::wstring &image_path) {
    FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeU(image_path.c_str(), 0);
    if (format == FIF_UNKNOWN) {
        format = FreeImage_GetFIFFromFilenameU(image_path.c_str());
    }
    if (format != FIF_UNKNOWN) {
        return FreeImage_LoadU(FreeImage_GetFileTypeU(image_path.c_str(), 0), image_path.c_str());
    } else {
        throw RuntimeError(std::format("无法确定图像\"{}\"的格式", nowide::narrow(image_path)));
    }
}
[[maybe_unused]] static FIBITMAP *freeimage_open_image(const std::string &image_path) {
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(image_path.c_str(), 0);
    if (format == FIF_UNKNOWN) {
        format = FreeImage_GetFIFFromFilename(image_path.c_str());
    }
    if (format != FIF_UNKNOWN) {
        return FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    } else {
        throw RuntimeError(std::format("无法确定图像\"{}\"的格式", image_path));
    }
}

static FIBITMAP *freeimage_load_and_convert_image(const std::filesystem::path &image_path, bool need_gammar_correct) {
    FIBITMAP *pImage = freeimage_open_image(image_path.native());
    if (pImage == nullptr) {
        throw RuntimeError(std::format("加载图像失败: {}", image_path.string()));
    }

    if (need_gammar_correct) {
        // 对于颜色贴图，进行矫正
        FreeImage_AdjustGamma(pImage, 1 / 2.2); // FreeImage的实现中用1/gamme，所以这里的1/2.2是对的
    }
    return pImage;
}

static TextureStorageFormat get_proper_storage_type(FIBITMAP *pImage) {
    switch (FreeImage_GetImageType(pImage)) {
    case FIT_UINT16: {
        return TextureStorageFormat::R_u16;
    }
    case FIT_INT16: {
        return TextureStorageFormat::R_i16;
    }
    case FIT_UINT32: {
        return TextureStorageFormat::R_u32;
    }
    case FIT_INT32: {
        return TextureStorageFormat::R_i32;
    }
    case FIT_FLOAT: {
        return TextureStorageFormat::R_f32;
    }
    case FIT_RGB16: {
        return TextureStorageFormat::RGB_f16;
    }
    case FIT_RGBA16: {
        return TextureStorageFormat::RGBA_f16;
    }
    case FIT_RGBF: {
        return TextureStorageFormat::RGB_f32;
    }
    case FIT_RGBAF: {
        return TextureStorageFormat::RGBA_f32;
    }
    case FIT_BITMAP:{
        unsigned int bpp = FreeImage_GetBPP(pImage);
        if (bpp == 24){
            return TextureStorageFormat::RGB_f8;
        } else if (bpp == 32){
            return TextureStorageFormat::RGBA_f8;
        }
    }
    case FIT_DOUBLE:
    case FIT_COMPLEX:
    case FIT_UNKNOWN:
        break;
    }
    return TextureStorageFormat::UNKNOWN;
}

void RenderReousce::add_texture2d(const std::string &key, const Texture2DDesc &desc) {
    LOG_INFO("Loading Texture: {}", key);
    FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path, desc.is_srgb);

    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    TextureStorageFormat storage_type = get_proper_storage_type(pImage);
    if (storage_type == TextureStorageFormat::UNKNOWN){
        throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", desc.path.string()));
    }
    TextureCreateDesc texture_desc{TextureType::TEXTURE_2D, storage_type, {nWidth, nHeight, 0}};

    intrusive_ptr<Texture> texture = graphics_api->create_texture(texture_desc);
    texture->set_filter_mode(desc.filter_mode);
    texture->set_warp_mode(desc.warp_mode);

    texture->import_image(pImage, 0);
    texture->generate_mipmaps();

    debug_check_error();

    FreeImage_Unload(pImage);

    textures.emplace(key, texture);
}

void RenderReousce::add_cubemap(const std::string &key, const TextureCubeMapDesc &desc) {
    LOG_INFO("Loading CubeMap: {}", key);
    // 使用第一张图像的宽高信息分配纹理空间
    FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path[0], desc.is_srgb);

    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    TextureStorageFormat storage_type = get_proper_storage_type(pImage);
    if (storage_type == TextureStorageFormat::UNKNOWN){
        throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", desc.path[0].string()));
    }
    TextureCreateDesc texture_desc{TextureType::TEXTURE_CUBEMAP, storage_type, {nWidth, nHeight, 0}};
    intrusive_ptr<Texture> texture = graphics_api->create_texture(texture_desc);
    texture->set_filter_mode(desc.filter_mode);
    texture->set_warp_mode(desc.warp_mode);

    texture->import_image(pImage, 0, 0, 0, 0);
    FreeImage_Unload(pImage);

    // 加载其余方向上的图像
    for (unsigned int i = 1; i < desc.path.size(); i++) {
        FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path[i], desc.is_srgb);
        if (nWidth != FreeImage_GetWidth(pImage) || nHeight != FreeImage_GetHeight(pImage)) {
            throw RuntimeError(std::format("CubeMap{}的大小不一致", desc.path[i].string()));
        }
        texture->import_image(pImage, 0, 0, 0, i);
        FreeImage_Unload(pImage);
    }
    texture->generate_mipmaps();

    debug_check_error();

    textures.emplace(key, texture);
};

} // namespace Graphics
} // namespace Goonya
