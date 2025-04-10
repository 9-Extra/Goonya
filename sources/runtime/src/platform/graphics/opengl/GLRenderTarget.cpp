#include "GLRenderTarget.h"
#include "core/intrusive_ptr.h"
#include "core/log/Log.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/graphics/opengl/GLTexture.h"
#include <cassert>
#include <spdlog/details/circular_q.h>
#include <variant>


namespace Goonya {
namespace Graphics {

// --------------------------GLRenderTargetScreen------------------------------------
void GLRenderTargetScreen::bind_draw() const {
    // 在绘制到屏幕上时，Y轴不需要翻转，以顺时针为正面
    glFrontFace(GL_CW);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
};
void GLRenderTargetScreen::bind_read() const { glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); };

// --------------------------GLFrameBuffer------------------------------------

GLFrameBuffer::GLFrameBuffer(std::tuple<uint32_t, uint32_t> size) : FrameBuffer(size) { glCreateFramebuffers(1, &id); }

void GLFrameBuffer::bind_read() const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    update_drawbuffers();
    
}
// 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
void GLFrameBuffer::bind_draw() const {
    // 在绘制到纹理上时，Y轴翻转（由透视投影矩阵完成），顶点环绕方向也反向了，所以改成逆时针
    glFrontFace(GL_CCW);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
    update_drawbuffers();
    
}

// 在不指定layer的情况下，如果Texture有多层（比如CubeMap有6层），就会形成多层帧缓冲，可用于多层渲染
void GLFrameBuffer::attach_color_texture(uint32_t location, intrusive_ptr<Texture> texture, int32_t level) {
    assert(texture);
    glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + location, ((GLTexture *)texture.get())->get_id(), level);
    attached_color_texture[location] = texture;
}
void GLFrameBuffer::attach_color_texture_layer(uint32_t location, intrusive_ptr<Texture> texture, int32_t layer,
                                                int32_t level) {
    assert(texture);
    glNamedFramebufferTextureLayer(id, GL_COLOR_ATTACHMENT0 + location, ((GLTexture *)texture.get())->get_id(), level,
                                   layer);
    attached_color_texture[location] = texture;
}

void GLFrameBuffer::detach_color_texture(uint32_t location) noexcept {
    glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + location, 0, 0);
    attached_color_texture[location] = nullptr;
};

// 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
void GLFrameBuffer::set_depth_texture(intrusive_ptr<Texture> texture, int32_t level) {
    glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    depth_buffer = texture;
}
void GLFrameBuffer::set_depth_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    glNamedFramebufferTextureLayer(id, GL_DEPTH_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level, layer);
    depth_buffer = texture;
}
void GLFrameBuffer::set_depth_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer = make_intrusive<GLRenderBuffer>(size, format);
    glNamedFramebufferRenderbuffer(id, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    depth_buffer = renderbuffer;
    
}

void GLFrameBuffer::set_stencil_texture(intrusive_ptr<Texture> texture, int32_t level) {
    assert(texture);
    glNamedFramebufferTexture(id, GL_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    stencil_buffer = texture;
}
void GLFrameBuffer::set_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    assert(texture);
    glNamedFramebufferTextureLayer(id, GL_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level, layer);
    stencil_buffer = texture;
}
void GLFrameBuffer::set_stencil_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer = make_intrusive<GLRenderBuffer>(size, format);
    glNamedFramebufferRenderbuffer(id, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    stencil_buffer = renderbuffer;
}

void GLFrameBuffer::set_depth_stencil_texture(intrusive_ptr<Texture> texture, int32_t level) {
    assert(texture);
    glNamedFramebufferTexture(id, GL_DEPTH_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level);
    depth_buffer = texture;
    stencil_buffer = texture;
}
void GLFrameBuffer::set_depth_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level) {
    assert(texture);
    glNamedFramebufferTextureLayer(id, GL_DEPTH_STENCIL_ATTACHMENT, ((GLTexture *)texture.get())->get_id(), level,
                                   layer);
    depth_buffer = texture;
    stencil_buffer = texture;
}
void GLFrameBuffer::set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) {
    assert(size != std::make_tuple(0, 0));
    intrusive_ptr<GLRenderBuffer> renderbuffer = make_intrusive<GLRenderBuffer>(size, format);
    glNamedFramebufferRenderbuffer(id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer->get_id());
    depth_buffer = renderbuffer;
    stencil_buffer = renderbuffer;
}

// 记得额外检查一下深度测试是否可以顺利进行
bool GLFrameBuffer::check_status() const noexcept {
    GLenum state = glCheckNamedFramebufferStatus(id, GL_DRAW_FRAMEBUFFER);
    switch (state) {
    case GL_FRAMEBUFFER_COMPLETE: {
        return true;
    };
    case GL_FRAMEBUFFER_UNDEFINED:
        LOG_ERROR("FBO状态错误：未定义");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        LOG_ERROR("FBO状态错误：关联的对象被删除");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        LOG_ERROR("FBO状态错误：至少关联一个对象");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        LOG_ERROR("FBO状态错误：指定的绘制目标中存在空对象");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        LOG_ERROR("FBO状态错误：指定的读取目标中存在空对象");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        LOG_ERROR("FBO状态错误：包含不支持的内部格式");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        LOG_ERROR("FBO状态错误：内部对象的多重采样样本数设定不一致，或者 GL_TEXTURE_FIXED_SAMPLE_LOCATIONS设定不一致");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        LOG_ERROR("FBO状态错误：有的对象是多层的但有的不是");
        break;
    default:
        LOG_ERROR("FBO状态错误：未知");
        break;
    }
    return false;
}

} // namespace Graphics
} // namespace Goonya
