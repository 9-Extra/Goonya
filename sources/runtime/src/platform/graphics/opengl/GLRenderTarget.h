#pragma once

#include "core/RefCount.h"
#include "platform/display/display.h"
#include "platform/graphics/opengl/GLTexture.h"

#include <cstddef>
#include <cstdint>

#include <array>
#include <glad/glad.h>
#include <tuple>
#include <variant>

namespace Goonya {

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

// 用作RenderTarget，类似纹理，但是无法进行采样（只写），但是性能更好
class GLRenderBuffer : public RefCount {
public:
    GLRenderBuffer(std::tuple<uint32_t, uint32_t> size, RenderBufferPixelFormat format) : size(size) {
        glCreateRenderbuffers(1, &id);
        auto [w, h] = size;
        glNamedRenderbufferStorage(id, BufferFormat2GL(format), w, h);
    }
    ~GLRenderBuffer() override { glDeleteRenderbuffers(1, &id); }

    GLuint get_id() const noexcept { return id; }
    std::tuple<uint32_t, uint32_t> get_size() const noexcept { return size; }

private:
    GLuint id{};

    std::tuple<uint32_t, uint32_t> size;

    static GLenum BufferFormat2GL(RenderBufferPixelFormat format) noexcept {
        switch (format) {
        case RenderBufferPixelFormat::DEPTH16:
            return GL_DEPTH_COMPONENT16;
        case RenderBufferPixelFormat::DEPTH24:
            return GL_DEPTH_COMPONENT24;
        case RenderBufferPixelFormat::DEPTH32:
            return GL_DEPTH_COMPONENT32;
        case RenderBufferPixelFormat::DEPTH32F:
            return GL_DEPTH_COMPONENT32F;
        case RenderBufferPixelFormat::STENCIL8:
            return GL_STENCIL_INDEX8;
        case RenderBufferPixelFormat::DEPTH24_STENCIL8:
            return GL_DEPTH24_STENCIL8;
        case RenderBufferPixelFormat::DEPTH32F_STENCIL8:
            return GL_DEPTH32F_STENCIL8;
        }
        return GL_NONE;
    }
};

// ========================GLFrameBuffer=====================

class GLRenderTargetScreen final : public RenderTarget {

    ~GLRenderTargetScreen() override = default;

    void bind_read() const override;
    void bind_draw() const override;

    std::tuple<uint32_t, uint32_t> get_size() const noexcept override {
        // OpenGL无法获取默认缓冲区的大小，使用GLFW
        return Display::get_size();
    };

    bool is_screen() const noexcept override { return true; }; // 是否指向屏幕

    bool has_color_buffer(uint32_t location) const noexcept override { return true; };
    bool has_depth_buffer() const noexcept override { return true; };
    bool has_stencil_buffer() const noexcept override { return true; /*按glfw的默认设置，有8位模板缓冲*/ };

    bool check_status() const noexcept override { return true; /* 如果存在则不会有问题*/ };

private:
    friend class OpenGLGraphicsAPI;
    // 由OpenGLGraphicsAPI创建单例
    GLRenderTargetScreen() {
        glNamedFramebufferDrawBuffer(0, GL_BACK); // 绑定后缓冲区
    }
};

class GLFrameBuffer final : public RenderTarget {
public:
    static const size_t MAX_ATTACH_COLOR = 8;

private:
    GLuint id = 0;

    std::tuple<uint32_t, uint32_t> size;

    std::array<Ref<GLTexture>, MAX_ATTACH_COLOR> attached_color_texture; // 至少支持8个，那就只最多支持8个
    std::variant<std::monostate, Ref<GLTexture>, Ref<GLRenderBuffer>>
        depth_buffer; // 可能是空的，也可能和stencil_buffer是同一个
    std::variant<std::monostate, Ref<GLTexture>, Ref<GLRenderBuffer>> stencil_buffer;

public:
    explicit GLFrameBuffer(std::tuple<uint32_t, uint32_t> size);
    ~GLFrameBuffer() { glDeleteFramebuffers(1, &id); }

    std::tuple<uint32_t, uint32_t> get_size() const noexcept override { return size; };
    bool is_screen() const noexcept override { return false; }

    void bind_read() const override;
    // 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
    void bind_draw() const override;

    void attach_color_texture(uint32_t location, Ref<GLTexture> texture, int32_t level = 0);
    void attach_color_texture_layer(uint32_t location, Ref<GLTexture> texture, int32_t layer, int32_t level = 0);
    void detach_color_texture(uint32_t location) noexcept;
    Ref<GLTexture> get_color_texture(uint32_t location) const noexcept { return attached_color_texture[location]; };
    bool has_color_buffer(uint32_t location) const noexcept override { return bool(attached_color_texture[location]); }
    // 不会有想要渲染到RenderBuffer的吧？不会吧不会吧

    // 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
    void set_depth_texture(Ref<GLTexture> texture, int32_t level = 0);
    void set_depth_texture_layer(Ref<GLTexture> texture, int32_t layer, int32_t level = 0);
    void set_depth_renderbuffer(RenderBufferPixelFormat format);
    bool has_depth_buffer() const noexcept override { return !std::holds_alternative<std::monostate>(depth_buffer); };

    void set_stencil_texture(Ref<GLTexture> texture, int32_t level = 0);
    void set_stencil_texture_layer(Ref<GLTexture> texture, int32_t layer, int32_t level = 0);
    void set_stencil_renderbuffer(RenderBufferPixelFormat format);
    bool has_stencil_buffer() const noexcept override {
        return !std::holds_alternative<std::monostate>(stencil_buffer);
    };

    void set_depth_stencil_texture(Ref<GLTexture> texture, int32_t level = 0);
    void set_depth_stencil_texture_layer(Ref<GLTexture> texture, int32_t layer, int32_t level = 0);
    void set_depth_stencil_renderbuffer(RenderBufferPixelFormat format);

    bool check_status() const noexcept override;

private:
    void update_drawbuffers() const noexcept {
        std::array<GLenum, MAX_ATTACH_COLOR> attachment; // NOLINT：后面会初始化
        for (GLenum i = 0; i < MAX_ATTACH_COLOR; i++) {
            attachment[i] = attached_color_texture[i] ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
        }
        glNamedFramebufferDrawBuffers(id, MAX_ATTACH_COLOR, attachment.data());
    }
};

} // namespace Goonya
