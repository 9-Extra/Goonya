#include "TextureArrayAllocator.h"

#include "core/RefCount.h"
#include "core/cgmath/vector.h"
#include "core/log/Log.h"
#include "craft/core/resource.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "platform/image/image.h"

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <span>

namespace Craft {

TextureArrayAllocator::TextureArrayAllocator(std::filesystem::path resource_path, uint32_t width, uint32_t height)
    : resource_path(std::move(resource_path)), width(width), height(height) {
    // 生成缺省图像，也就是紫黑块
    stb::Image missing_image = stb::Image::create_empty(width, height, 4, true);
    Goonya::Vector4f black{0, 0, 0, 1};
    Goonya::Vector4f purple{0.68542f, 0.3735f, 0.803862f, 1.0f};
    std::span<Goonya::Vector4f> pixel_view((Goonya::Vector4f *)missing_image.get_data(), width * height);
    // 紫黑块
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            if ((i < height / 2) != (j < width / 2)) {
                pixel_view[i * width + j] = purple;
            } else {
                pixel_view[i * width + j] = black;
            }
        }
    }
    texture_storage.emplace_back(std::move(missing_image)); // 0号图像为缺省图像
    texture_index_cache.emplace("", 0);                     // key为空视为缺省图像
}

uint32_t TextureArrayAllocator::alloc_texture(std::string_view texture_location) {
    if (auto iter = texture_index_cache.find(texture_location); iter != texture_index_cache.end()) {
        return iter->second;
    }

    ResourceLocation location = ResourceLocation::parse(texture_location);

    std::filesystem::path real_path =
        resource_path / std::format("{}/textures/{}.png", location.name_space, location.key);
    stb::Image image = stb::Image::loadf(real_path, true);
    if (!image) {
        LOG_ERROR("加载图像{}失败，使用缺省图像", real_path.generic_string());
        return 0;
    }
    if (image.get_width() != (int)width || image.get_height() != (int)height) {
        LOG_ERROR("图像{}大小与预设大小不匹配，使用缺省图像", real_path.generic_string());
        return 0;
    }

    uint32_t id = (uint32_t)texture_storage.size();
    texture_storage.emplace_back(std::move(image));
    texture_index_cache.emplace(texture_location, id);

    return id;
}

Ref<Goonya::GLTexture> TextureArrayAllocator::generate_texture_array() {
    uint32_t texture_count = (uint32_t)texture_storage.size();
    Ref<Goonya::GLTexture> texture_array =
        create_ref<Goonya::GLTexture>(Goonya::TextureType::TEXTURE_2D_ARRAY, Goonya::TextureStorageFormat::RGBA_f16,
                                      std::make_tuple(this->width, this->height, texture_count));
    for (uint32_t i = 0; i < texture_count; i++) {
        texture_array->import_image(texture_storage[i], 0, 0, 0, i);
    }

    texture_array->set_filter_mode(Goonya::TextureFilterMode::NEAREST);

    return texture_array;
}

} // namespace Craft
