#pragma once

#include "core/intrusive_ptr.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/Texture.h"
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

class GLRenderTarget : public RenderTarget {
public:
    GLRenderTarget(std::tuple<uint32_t, uint32_t> size = {0, 0});
    virtual ~GLRenderTarget() { glad_glDeleteFramebuffers(1, &id); }

    virtual void bind_read() const override;
    // 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
    virtual void bind_draw() const override;

    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept override { return size; }

    // 在不指定layer的情况下，如果Texture有多层（比如CubeMap有6层），就会形成多层帧缓冲，可用于多层渲染
    virtual void attach_color_texture(uint32_t location, intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void attach_color_texture_layer(uint32_t location, intrusive_ptr<Texture> texture, int32_t layer,
                                            int32_t level = 0) override;
    // 不会有想要渲染到RenderBuffer的吧？不会吧不会吧

    // 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
    virtual void set_depth_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_depth_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) override;
    virtual void set_depth_renderbuffer(RenderBufferPixelFormat format) override;

    virtual void set_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) override;
    virtual void set_stencil_renderbuffer(RenderBufferPixelFormat format) override;

    virtual void set_depth_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) override;
    virtual void set_depth_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer,
                                                 int32_t level = 0) override;
    virtual void set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) override;

    // 记得额外检查一下深度测试是否可以顺利进行
    virtual bool check_status() const noexcept override;

private:
    static constexpr size_t MAX_ATTACH_COLOR = 8;
    GLuint id;

    std::tuple<uint32_t, uint32_t> size;
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