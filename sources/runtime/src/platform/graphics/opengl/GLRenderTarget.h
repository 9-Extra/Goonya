#pragma once

#include "core/intrusive_ptr.h"
#include "platform/display/display.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/opengl/GLBasic.h"
#include <cstddef>
#include <cstdint>

#include <array>
#include <glad/glad.h>
#include <tuple>
#include <variant>

namespace Goonya {
namespace Graphics {

// 用作RenderTarget，类似纹理，但是无法进行采样（只写），但是性能更好
class GLRenderBuffer : public RenderBuffer {
public:
    GLRenderBuffer(std::tuple<uint32_t, uint32_t> size, RenderBufferPixelFormat format): size(size) {
        glCreateRenderbuffers(1, &id);
        auto [w, h] = size;
        glNamedRenderbufferStorage(id, BufferFormat2GL(format), w, h);
    }
    virtual ~GLRenderBuffer() { glDeleteRenderbuffers(1, &id); }
    
    GLuint get_id() const noexcept{
        return id;
    }
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept { return size; }
private:
    GLuint id;

    std::tuple<uint32_t, uint32_t> size;

    static GLenum BufferFormat2GL(RenderBufferPixelFormat format) noexcept{
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

class GLRenderTargetScreen final : public RenderTarget{
    
    virtual ~GLRenderTargetScreen() = default;

    virtual void bind_read() const;
    virtual void bind_draw() const;

    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept {
        // OpenGL无法获取默认缓冲区的大小，使用GLFW
        return Display::get_size();
    };
    
    virtual bool is_screen() const noexcept {return true;}; // 是否指向屏幕
    
    virtual bool has_color_buffer(uint32_t location) const noexcept {return true;};
    virtual bool has_depth_buffer() const noexcept {return true;};
    virtual bool has_stencil_buffer() const noexcept  {return true; /*按glfw的默认设置，有8位模板缓冲*/ };
    
    virtual bool check_status() const noexcept {return true; /* 如果存在则不会有问题*/};

private:
    friend class OpenGLGraphicsAPI;
    // 由OpenGLGraphicsAPI创建单例
    GLRenderTargetScreen() {
        glNamedFramebufferDrawBuffer(0, GL_BACK); // 绑定后缓冲区
    }
};

class GLFrameBuffer final : public FrameBuffer {
public:
    GLFrameBuffer(std::tuple<uint32_t, uint32_t> size);
    virtual ~GLFrameBuffer() { glDeleteFramebuffers(1, &id); }

    virtual void bind_read() const override;
    // 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
    virtual void bind_draw() const override;

    virtual void attach_color_texture(uint32_t location, intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void attach_color_texture_layer(uint32_t location, intrusive_ptr<Texture> texture, int32_t layer,
                                            int32_t level = 0) override;
    virtual void detach_color_texture(uint32_t location) noexcept override;
    virtual intrusive_ptr<Texture> get_color_texture(uint32_t location) const noexcept override{
        return attached_color_texture[location];
    };
    // 不会有想要渲染到RenderBuffer的吧？不会吧不会吧

    // 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
    virtual void set_depth_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_depth_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) override;
    virtual void set_depth_renderbuffer(RenderBufferPixelFormat format) override;
    virtual bool has_depth_buffer() const noexcept override{
        return !std::holds_alternative<std::monostate>(depth_buffer);
    };

    virtual void set_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) override;
    virtual void set_stencil_renderbuffer(RenderBufferPixelFormat format) override;
    virtual bool has_stencil_buffer() const noexcept override{
        return !std::holds_alternative<std::monostate>(stencil_buffer);
    };

    virtual void set_depth_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_depth_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer,
                                                 int32_t level = 0) override;
    virtual void set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) override;

    virtual bool check_status() const noexcept override;

private:
    static constexpr size_t MAX_ATTACH_COLOR = 8;
    GLuint id;

    std::array<intrusive_ptr<Texture>, MAX_ATTACH_COLOR> attached_color_texture; // 至少支持8个，那就只最多支持8个
    std::variant<std::monostate, intrusive_ptr<Texture>, intrusive_ptr<RenderBuffer>>
        depth_buffer; // 可能是空的，也可能和stencil_buffer是同一个
    std::variant<std::monostate, intrusive_ptr<Texture>, intrusive_ptr<RenderBuffer>> stencil_buffer;

    void update_drawbuffers() const noexcept{
        std::array<GLenum, MAX_ATTACH_COLOR> attachment;
        for (size_t i = 0; i < MAX_ATTACH_COLOR; i++) {
            attachment[i] = attached_color_texture[i] ? GL_COLOR_ATTACHMENT0 + i : GL_NONE;
        }
        glNamedFramebufferDrawBuffers(id, MAX_ATTACH_COLOR, attachment.data());
    }
};

} // namespace Graphics
} // namespace Goonya