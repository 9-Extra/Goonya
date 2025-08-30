#pragma once

#include "core/hash_helper.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Texture.h"

#include "platform/image/image.h"
#include <cstdint>
#include <filesystem>
#include <string>

namespace Craft {

class TextureArrayAllocator {
private:
    std::filesystem::path resource_path;
    uint32_t width;
    uint32_t height;

    std::unordered_map<std::string, uint32_t, Goonya::StringHash, Goonya::StringEqual> texture_index_cache;
    std::vector<stb::Image> texture_storage;

public:
    TextureArrayAllocator(TextureArrayAllocator&) = delete;
    TextureArrayAllocator(std::filesystem::path resource_path, uint32_t width, uint32_t height);
    uint32_t alloc_texture(std::string_view texture_location);
    intrusive_ptr<Goonya::Graphics::Texture> generate_texture_array();
};

} // namespace Craft