#include "image.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <stb/stb_image_write.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>

#include <filesystem>

namespace stb {

Image Image::flip_vertical() const noexcept {
    if (!*this) {
        // 空的图像不能操作
        return Image{};
    }

    Image image;
    image.width = width;
    image.height = height;
    image.channel = channel;
    image._is_float = _is_float;
    image.data = malloc(get_size_byte());

    unsigned int line_stripe = width * channel * (_is_float ? 4 : 1);
    for (int i = 0; i < height; i++) {
        memcpy((uint8_t *)image.data + i * line_stripe, (uint8_t *)data + (height - i - 1) * line_stripe, line_stripe);
    }
    return image;
}

Image Image::resize(int target_width, int target_height, ResizeMethod method) const noexcept {
    if (!*this) {
        // 空的图像不能操作
        return Image{};
    }

    assert(false); // todo
    return Image{};
};

int Image::save(const std::filesystem::path &path) const {
    int result = -1;
    std::string path_str = path.string();
    std::string ext = path.extension().string();
    if (ext == ".bmp") {
        assert(!is_float());
        result = stbi_write_bmp(path_str.c_str(), width, height, channel, data);
    } else if (ext == ".png") {
        assert(!is_float());
        result = stbi_write_png(path_str.c_str(), width, height, channel, data, width * channel);
    } else if (ext == ".tga") {
        assert(!is_float());
        result = stbi_write_tga(path_str.c_str(), width, height, channel, data);
    } else if (ext == ".jpg") {
        assert(!is_float());
        result = stbi_write_jpg(path_str.c_str(), width, height, channel, data, 90);
    } else if (ext == ".hdr") {
        assert(is_float());
        result = stbi_write_hdr(path_str.c_str(), width, height, channel, (float *)data);
    }
    return result;
}

// 预先计算Gamma查找表 (LUT)
const static std::array<uint8_t, 256> gamma_lut = [] {
    std::array<uint8_t, 256> lut{};
    constexpr float gamma = 2.2f;
    for (int i = 0; i < 256; ++i) {
        lut[i] = static_cast<uint8_t>(std::clamp(std::pow(i / 255.0f, gamma) * 255.0f + 0.5f, // +0.5f用于四舍五入
                                                 0.0f, 255.0f));
    }
    return lut;
}();

Image Image::load(const std::filesystem::path &path, bool to_srgb_linear) {
    // STB加载的图像存放方式默认是从上到下的，原点在左上角
    // 理想情况下颜色图像使用线性空间及hdr存储，非颜色图像使用非hdr方式存储
    // 但实际上很多时候颜色图像也使用sRGB空间，非hdr方式
    Image image;
    std::string path_str = path.string();
    if (stbi_is_hdr(path_str.c_str())) {
        assert(!to_srgb_linear); // 不应该使用hdr格式存储非颜色信息
        // 会自动转换到线性空间（gamma变换）
        image.data = stbi_loadf(path_str.c_str(), &image.width, &image.height, &image.channel, 0);
        image._is_float = true;
    } else {
        // 不会自动转换，因此如果需要颜色数据则手动转换
        image.data = stbi_load(path_str.c_str(), &image.width, &image.height, &image.channel, 0);
        image._is_float = false;
        if (image.data == nullptr) {
            return image;
        }
        if (to_srgb_linear) {
            uint8_t *data_ptr = reinterpret_cast<uint8_t *>(image.data);
            const size_t size = image.get_size_byte();
            // 基于查找表的gamma变换
            for (size_t i = 0; i < size; ++i) {
                data_ptr[i] = gamma_lut[data_ptr[i]];
            }
        }
    }
    return image;
}

Image Image::loadf(const std::filesystem::path &path, bool to_srgb_linear){
    Image image;
    assert(to_srgb_linear == true); // stb 内部写死的。。。。。
    image.data = stbi_loadf(path.string().c_str(), &image.width, &image.height, &image.channel, 0);
    image._is_float = true;
    return image;
}

Image Image::create_empty(int width, int height, int channel, bool _is_float) {
    Image image;
    image.width = width;
    image.height = height;
    image.channel = channel;
    image._is_float = _is_float;
    image.data = malloc(image.get_size_byte());
    memset(image.data, 0, image.get_size_byte());

    return image;
}

Image::~Image() { stbi_image_free(data); }
} // namespace stb