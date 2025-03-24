#pragma once

#include "core/intrusive_ptr.h"
#include "platform/graphics/Texture.h"
#include <cstdint>
#include <tuple>

namespace Goonya {
namespace Graphics {

// ===============RenderBuffer=============
enum class RenderBufferPixelFormat{
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
class RenderBuffer: public intrusive_ptr_base<RenderBuffer>{
public:
    virtual ~RenderBuffer() = default;
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept = 0; // (weight, height)   
};

class RenderTarget: public intrusive_ptr_base<RenderTarget>{
public:
    virtual ~RenderTarget() = default;
    
    virtual void bind_read() const = 0;
    // 忽略glDrawBuffers的再次重定向，在绑定时直接将所有关联的颜色缓冲按照attachment用作渲染目标
    virtual void bind_draw() const = 0;
    // 没找到就返回0x0，如果内部纹理大小不一致会有错误结果
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept = 0;

    // 在不指定layer的情况下，如果Texture有多层（比如CubeMap有6层），就会形成多层帧缓冲，可用于多层渲染
    virtual void attach_color_texture(uint32_t location, intrusive_ptr<Texture> texture, int32_t level = 0) = 0;
    virtual void attach_color_texture_layer(uint32_t location, intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) = 0;
    
    // 反正renderbuffer不能读，所有直接在内部创建，内部使用。如果要读则使用Texture
    virtual void set_depth_texture(intrusive_ptr<Texture> texture, int32_t level = 0) = 0;
    virtual void set_depth_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) = 0;
    virtual void set_depth_renderbuffer(RenderBufferPixelFormat format) = 0;

    virtual void set_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) = 0;
    virtual void set_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) = 0;
    virtual void set_stencil_renderbuffer(RenderBufferPixelFormat format) = 0;

    virtual void set_depth_stencil_texture(intrusive_ptr<Texture> texture, int32_t level = 0) = 0;
    virtual void set_depth_stencil_texture_layer(intrusive_ptr<Texture> texture, int32_t layer, int32_t level = 0) = 0;
    virtual void set_depth_stencil_renderbuffer(RenderBufferPixelFormat format) = 0;

    // 记得额外检查一下深度测试是否可以顺利进行
    virtual bool check_status() const noexcept = 0;
};


}
}