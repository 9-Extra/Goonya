#include "TextureLoader.h"

#include "core/path_formatter.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Texture.h"
#include "platform/image/image.h"
#include "runtime/GoonyaException.h"

namespace Goonya {

static std::tuple<Graphics::TextureFilterMode, Graphics::TextureWarpMode>
parse_texture_profile(const Json::Value &texture_desc) {
    Graphics::TextureFilterMode filter_mode;
    Graphics::TextureWarpMode warp_mode;
    const Json::Value &filter_mode_name = texture_desc["filter_mode"];
    const Json::Value &warp_mode_name = texture_desc["warp_mode"];

    if (!filter_mode_name || filter_mode_name == "trilinear") {
        filter_mode = Graphics::TextureFilterMode::TRILINEAR;
    } else if (filter_mode_name == "point") {
        filter_mode = Graphics::TextureFilterMode::BILINEAR;
    } else if (filter_mode_name == "bilinear") {
        filter_mode = Graphics::TextureFilterMode::NEAREST;
    } else {
        throw RuntimeError(std::format("未知的纹理过滤模式：{}", filter_mode_name.asString()));
    }

    if (!warp_mode_name || warp_mode_name == "repeat") {
        warp_mode = Graphics::TextureWarpMode::REPEAT;
    } else if (warp_mode_name == "clamp") {
        warp_mode = Graphics::TextureWarpMode::ClAMP;
    } else if (warp_mode_name == "mirror") {
        warp_mode = Graphics::TextureWarpMode::MIRROR;
    } else {
        throw RuntimeError(std::format("未知的纹理重复模式：{}", warp_mode_name.asString()));
    }

    return {filter_mode, warp_mode};
}

Ref<Resource> TextureLoader::load(std::string_view type, const std::filesystem::path &base_dir, std::string_view name,
                                  const Json::Value &content) {
    if (type == "Texture") {
        const Json::Value &texture_desc = content;

        std::filesystem::path image_path = base_dir / as_u8string_view(texture_desc["image"].asString());
        bool is_color = texture_desc.get("is_color", false).asBool();

        stb::Image image = stb::Image::load(image_path, is_color);
        if (!image) {
            throw RuntimeError(std::format("图像{}加载失败", image_path));
        }

        uint32_t width = image.get_width();
        uint32_t height = image.get_height();

        Graphics::TextureStorageFormat storage_type = Graphics::Texture::get_proper_storage_type(image);

        if (storage_type == Graphics::TextureStorageFormat::UNKNOWN) {
            throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", image_path));
        }
        Graphics::TextureCreateDesc create_desc{Graphics::TextureType::TEXTURE_2D, storage_type, {width, height, 0}};

        Ref<Graphics::Texture> texture = Graphics::graphics_api->create_texture(create_desc);

        auto [filter_mode, warp_mode] = parse_texture_profile(texture_desc);
        texture->set_filter_mode(filter_mode);
        texture->set_warp_mode(warp_mode);

        texture->import_image(image, 0);
        texture->generate_mipmaps();

        return texture;
    } else if (type == "CubeMap") {
        const Json::Value &cubemap_desc = content;

        std::array<std::filesystem::path, 6> image_dirs = {
            base_dir / cubemap_desc["px"].asString(), base_dir / cubemap_desc["nx"].asString(),
            base_dir / cubemap_desc["py"].asString(), base_dir / cubemap_desc["ny"].asString(),
            base_dir / cubemap_desc["pz"].asString(), base_dir / cubemap_desc["nz"].asString()};

        bool is_color = cubemap_desc.get("is_color", false).asBool();
        auto [filter_mode, warp_mode] = parse_texture_profile(cubemap_desc);

        using Graphics::TextureStorageFormat;
        // 使用第一张图像的宽高信息分配纹理空间
        stb::Image image = stb::Image::load(image_dirs[0], is_color);
        if (!image) {
            throw RuntimeError(std::format("图像{}加载失败", image_dirs[0]));
        }

        int width = image.get_width();
        int height = image.get_height();

        TextureStorageFormat storage_type = Graphics::Texture::get_proper_storage_type(image);
        if (storage_type == TextureStorageFormat::UNKNOWN) {
            throw RuntimeError(std::format("不支持此图像像素格式\"{}\"", image_dirs[0]));
        }
        Graphics::TextureCreateDesc texture_desc{
            Graphics::TextureType::TEXTURE_CUBEMAP, storage_type, {(uint32_t)width, (uint32_t)height, 0}};
        Ref<Graphics::Texture> texture = Graphics::graphics_api->create_texture(texture_desc);
        texture->set_filter_mode(filter_mode);

        texture->import_image(image, 0, 0, 0, 0);

        // 加载其余方向上的图像
        for (unsigned int i = 1; i < image_dirs.size(); i++) {
            stb::Image image = stb::Image::load(image_dirs[i], is_color);
            if (!image) {
                throw RuntimeError(std::format("图像{}加载失败", image_dirs[i].string()));
            }
            if (width != image.get_width() || height != image.get_height()) {
                throw RuntimeError(std::format("CubeMap{}的大小不一致", image_dirs[i].string()));
            }
            texture->import_image(image, 0, 0, 0, i);
        }
        texture->generate_mipmaps();

        return texture;
    } else {
        throw RuntimeError(std::format("未知图像类型{}", type));
    }
}

} // namespace Goonya