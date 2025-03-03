#include "GLRenderTarget.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "core/log/Log.h"
#include <spdlog/details/circular_q.h>
#include <variant>

namespace Goonya {
namespace Graphics {

GLRenderTarget::GLRenderTarget(std::tuple<uint32_t, uint32_t> size) : size(size) { glCreateFramebuffers(1, &id); }

void GLRenderTarget::bind_read() const { glBindFramebuffer(GL_READ_FRAMEBUFFER, id); }
// 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
void GLRenderTarget::bind_draw() const { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); }

// 在不指定layer的情况下，如果Texture有多层（比如CubeMap有6层），就会形成多层帧缓冲，可用于多层渲染
void GLRenderTarget::attach_color_texture(uint32_t attachment, intrusive_ptr<Texture> texture, int32_t level) {
    glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + attachment, ((GLTexture *)texture.get())->get_id(), level);
    attached_color_texture[attachment] = texture;
    update_drawbuffers();
}
void GLRenderTarget::attach_color_texture_layer(uint32_t attachment, intrusive_ptr<Texture> texture, int32_t layer,
                                                int32_t level) {
    glNamedFramebufferTextureLayer(id, GL_COLOR_ATTACHMENT0 + attachment, ((GLTexture *)texture.get())->get_id(), level,
                                   layer);
    attached_color_texture[attachment] = texture;
    update_drawbuffers();
}

// 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
void GLRenderTarget::set_depth_texture(intrusive_ptr<Texture> texture, int32_t level) {
    glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    depth_buffer = texture;
}
void GLRenderTarget::set_depth_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    glNamedFramebufferTextureLayer(id, GL_DEPTH_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level, layer);
    depth_buffer = texture;
}
void GLRenderTarget::set_depth_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer(size, format);
    glNamedFramebufferRenderbuffer(id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    depth_buffer = renderbuffer;
}

void GLRenderTarget::set_stencil_texture(intrusive_ptr<Texture> texture, int32_t level) {
    glNamedFramebufferTexture(id, GL_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    stencil_buffer = texture;
}
void GLRenderTarget::set_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    glNamedFramebufferTextureLayer(id, GL_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level, layer);
    stencil_buffer = texture;
}
void GLRenderTarget::set_stencil_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer(size, format);
    glNamedFramebufferRenderbuffer(id, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    stencil_buffer = renderbuffer;
}

void GLRenderTarget::set_depth_stencil_texture(intrusive_ptr<Texture> texture, int32_t level) {
    glNamedFramebufferTexture(id, GL_DEPTH_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    depth_buffer = texture;
    stencil_buffer = texture;
}
void GLRenderTarget::set_depth_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    glNamedFramebufferTextureLayer(id, GL_DEPTH_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level,
                                   layer);
    depth_buffer = texture;
    stencil_buffer = texture;
}
void GLRenderTarget::set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer(size, format);
    glNamedFramebufferRenderbuffer(id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    depth_buffer = renderbuffer;
    stencil_buffer = renderbuffer;
}

// 记得额外检查一下深度测试是否可以顺利进行
bool GLRenderTarget::check_status() const noexcept {
    GLenum state = glCheckNamedFramebufferStatus(id, 0);
    switch (state) {
    case GL_FRAMEBUFFER_COMPLETE: {
        if (glIsEnabled(GL_DEPTH_TEST) && std::holds_alternative<std::monostate>(depth_buffer)) {
            LOG_WARN("启用了深度测试但是FBO里没有深度缓冲");
        }
        return true;
    };
    case GL_FRAMEBUFFER_UNDEFINED:
        LOG_ERROR("FBO状态错误：未定义");
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        LOG_ERROR("FBO状态错误：关联的对象被删除");
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        LOG_ERROR("FBO状态错误：至少关联一个对象");
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        LOG_ERROR("FBO状态错误：指定的绘制目标中存在空对象");
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        LOG_ERROR("FBO状态错误：指定的读取目标中存在空对象");
    case GL_FRAMEBUFFER_UNSUPPORTED:
        LOG_ERROR("FBO状态错误：包含不支持的内部格式");
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        LOG_ERROR("FBO状态错误：内部对象的多重采样样本数设定不一致，或者 GL_TEXTURE_FIXED_SAMPLE_LOCATIONS设定不一致");
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        LOG_ERROR("FBO状态错误：有的对象是多层的但有的不是");
    }
    return false;
}

} // namespace Graphics
} // namespace Goonya
