#pragma once

#include "../Graphics.h"
#include "GLMaterial.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLRenderTarget.h"
#include <memory>


namespace Goonya {
namespace Graphics {

class OpenGLGraphicsAPI final: public GraphicsAPI {
public:
    OpenGLGraphicsAPI();
    ~OpenGLGraphicsAPI() {
        // nothing
    }
    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------ 
    virtual intrusive_ptr<Mesh> load_mesh(const MeshDesc& desc) override;
    virtual intrusive_ptr<Texture> create_texture(const TextureCreateDesc &desc) const override{
        return intrusive_ptr<GLTexture>{desc};
    };
    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) override;
    virtual intrusive_ptr<FrameBuffer> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) override;
    
    
    // ---------------------------------材质和着色器相关------------------------------------------
    virtual intrusive_ptr<Shader> complie_shader_program(const std::string& vs_src, const std::string& ps_src) const override;
    virtual std::unique_ptr<ShaderIntrospector> create_shader_introspect(Shader* shader) const override{
        return std::make_unique<GLShaderIntrospector>(shader);
    }

    virtual intrusive_ptr<Material> create_material(UberShader* uber_shader) override{
        return intrusive_ptr<Material>{new GLMaterial{uber_shader}};
    }
    // ---------------------------------绘制调用-------------------------------------------------------------
    virtual intrusive_ptr<RenderTarget> get_rendertarget_screen() noexcept override {return rendertarget_screen;};
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                                     std::optional<int> stencil = std::nullopt) noexcept override;
    virtual void clear(bool color = true, bool depth = true, bool stencil = true) const noexcept override;
    virtual void draw(intrusive_ptr<Mesh> mesh) override;

    virtual void set_viewport(const Viewport& view_port) noexcept override;
    
    // --------------------其他------------------------------
    virtual Matrix4 compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                               bool render_to_texture = false) const noexcept override;

private:
    intrusive_ptr<RenderTarget> rendertarget_screen;
};

} // namespace Graphics

} // namespace Goonya