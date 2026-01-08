/*
    stb的c++风格封装
*/
#pragma once

#include <cassert>
#include <filesystem>

namespace stb {

enum class ResizeMethod {

};

class Image final { // NOLINT 不需要初始化
private:
    void *data = nullptr;
    int width, height, channel;
    bool _is_float;

    Image() = default; // NOLINT 不要直接构造，不需要初始化

public:
    Image(Image &&img) noexcept {
        data = img.data;
        width = img.width;
        height = img.height;
        channel = img.channel;
        _is_float = img._is_float;

        img.data = nullptr;
    }

    Image(const Image &img) = delete; // 不要拷贝

    Image &operator=(Image &&img) noexcept {
        if (this == &img) {
            return *this;
        }

        data = img.data;
        width = img.width;
        height = img.height;
        channel = img.channel;
        _is_float = img._is_float;

        img.data = nullptr;
        return *this;
    }

    Image &operator=(Image &img) = delete;

    explicit operator bool() const noexcept { return data != nullptr; }

    int get_width() const noexcept { return width; }
    int get_height() const noexcept { return height; }
    int get_channel() const noexcept { return channel; }
    bool is_float() const noexcept { return _is_float; }
    void *get_data() const noexcept { return data; }

    unsigned int get_size_byte() const noexcept { return width * height * channel * (_is_float ? 4 : 1); }

    Image flip_vertical() const noexcept;
    Image resize(int target_width, int target_height, ResizeMethod method) const noexcept;

    int save(const std::filesystem::path &path) const;
    static Image load(const std::filesystem::path &path, bool to_srgb_linear);
    static Image loadf(const std::filesystem::path &path, bool to_srgb_linear);
    static Image create_empty(int width, int height, int channel, bool _is_float);

    ~Image();
};

} // namespace stb
