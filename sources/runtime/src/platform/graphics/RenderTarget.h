#pragma once

#include "core/RefCount.h"
#include "platform/graphics/Texture.h"
#include <tuple>

namespace Goonya::Graphics {

struct Viewport {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

// ===============RenderBuffer=============
enum class RenderBufferPixelFormat {
    // 只用于深度
    DEPTH16,
    DEPTH24,
    DEPTH32,
    DEPTH32F,
    // 只用于模板
    STENCIL8,
    // 同时
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8,
};

// 用作RenderTarget，类似纹理，但是无法进行采样（只写），但是性能更好
class RenderBuffer : public RefCount {
public:
    virtual ~RenderBuffer() = default;
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept = 0; // (weight, height)
};

// ===================Render Target=======================
/**
 * @brief 可以绑定到渲染管线上的绘制目标
 *
 * 可能指向屏幕，此时其大小就是屏幕大小，也可能是FrameBuffer。
 */
class RenderTarget : public RefCount {
public:
    virtual ~RenderTarget() = default;
    RenderTarget(const RenderTarget &) = delete;
    RenderTarget(RenderTarget &&) = delete;

    virtual void bind_read() const = 0;
    virtual void bind_draw() const = 0;

    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept = 0;

    virtual bool is_screen() const noexcept = 0; // 是否指向屏幕

    virtual bool has_color_buffer(uint32_t location) const noexcept = 0;
    virtual bool has_depth_buffer() const noexcept = 0;
    virtual bool has_stencil_buffer() const noexcept = 0;

    virtual bool check_status() const noexcept = 0;

protected:
    RenderTarget() = default;
};

class FrameBuffer : public RenderTarget {
protected:
    std::tuple<uint32_t, uint32_t> size;

public:
    std::tuple<uint32_t, uint32_t> get_size() const noexcept override { return size; };
    bool is_screen() const noexcept override { return false; }

    // 在不指定layer的情况下，如果Texture有多层（比如CubeMap有6层），就会形成多层帧缓冲，可用于多层渲染
    // 传递空指针以解除关联
    virtual void attach_color_texture(uint32_t location, Ref<Texture> texture, int32_t level) = 0;
    virtual void attach_color_texture_layer(uint32_t location, Ref<Texture> texture, int32_t layer, int32_t level) = 0;
    virtual void detach_color_texture(uint32_t location) noexcept = 0;
    virtual Ref<Texture> get_color_texture(uint32_t location) const noexcept = 0;
    bool has_color_buffer(uint32_t location) const noexcept override {
        return static_cast<bool>(get_color_texture(location));
    };

    // 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
    virtual void set_depth_texture(Ref<Texture> texture, int32_t level) = 0;
    virtual void set_depth_texture_layer(Ref<Texture> texture, int32_t layer, int32_t level) = 0;
    virtual void set_depth_renderbuffer(RenderBufferPixelFormat format) = 0;

    virtual void set_stencil_texture(Ref<Texture> texture, int32_t level) = 0;
    virtual void set_stencil_texture_layer(Ref<Texture> texture, int32_t layer, int32_t level) = 0;
    virtual void set_stencil_renderbuffer(RenderBufferPixelFormat format) = 0;

    virtual void set_depth_stencil_texture(Ref<Texture> texture, int32_t level) = 0;
    virtual void set_depth_stencil_texture_layer(Ref<Texture> texture, int32_t layer, int32_t level) = 0;
    virtual void set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) = 0;

protected:
    explicit FrameBuffer(std::tuple<uint32_t, uint32_t> size) noexcept : size(size) {} // NOLINT
};

} // namespace Goonya::Graphics
