#pragma once

#include "../Graphics.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLRenderTarget.h"

#include <memory>


namespace Goonya {
namespace Graphics {

class OpenGLGraphicsAPI final: public GraphicsAPI {
public:
    OpenGLGraphicsAPI();
    ~OpenGLGraphicsAPI();

    // ---------------------------调试-----------------------------
    virtual void push_debug_group_label(const std::string& label) const noexcept override{
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, label.size(), label.data()); 
    };
    virtual void pop_debug_group_label() const noexcept override{
        glPopDebugGroup();
    }

    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------ 
    virtual intrusive_ptr<Mesh> create_mesh() override;
    virtual intrusive_ptr<Texture> create_texture(const TextureCreateDesc &desc) const override{
        return make_intrusive<GLTexture>(desc);
    };
    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) override;
    virtual intrusive_ptr<FrameBuffer> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) override;
    
    
    // ---------------------------------材质和着色器相关------------------------------------------
    virtual intrusive_ptr<Shader> complie_shader_program(const std::string& vs_src, const std::string& ps_src) const override;
    virtual std::unique_ptr<ShaderIntrospector> create_shader_introspect(Shader* shader) const override{
        return std::make_unique<GLShaderIntrospector>(shader);
    }

    virtual void set_pipeline_state(const PipeLineState &state) const noexcept override;

    // ---------------------------------绘制调用-------------------------------------------------------------
    virtual intrusive_ptr<RenderTarget> get_rendertarget_screen() noexcept override {return rendertarget_screen;};
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                                     std::optional<int> stencil = std::nullopt) noexcept override;
    virtual void clear(bool color = true, bool depth = true, bool stencil = true) const noexcept override;
    virtual void draw_submesh(const SubMesh& submesh) const override;

    virtual void set_viewport(const Viewport& view_port) noexcept override;
    
    // --------------------其他------------------------------
    virtual Matrix4 compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                               bool render_to_texture = false) const noexcept override;

private:
    intrusive_ptr<RenderTarget> rendertarget_screen;
};

} // namespace Graphics

} // namespace Goonya