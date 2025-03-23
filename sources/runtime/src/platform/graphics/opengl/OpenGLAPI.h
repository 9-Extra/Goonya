#pragma once

#include "../graphics.h"
#include "GLMaterial.h"
#include "GLBasic.h"
#include "GLShader.h"
#include <memory>


namespace Goonya {
namespace Graphics {

class OpenGLGraphicsAPI : public GraphicsAPI {
public:
    OpenGLGraphicsAPI();
    ~OpenGLGraphicsAPI() {
        // nothing
    }

    virtual void _check_error(const char* file, size_t line) override{
        _opengl_check_error(file, line);
    }
    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------ 
    virtual intrusive_ptr<Mesh> load_mesh(Topology topology, const VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) override;
    virtual intrusive_ptr<Texture> load_texture2D(const Texture2DDesc &desc) const override;
    virtual intrusive_ptr<Texture> load_cubemap(const TextureCubeMapDesc& desc) const override;

    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) override;
    virtual intrusive_ptr<RenderTarget> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) override;
    
    
    // ---------------------------------材质和着色器相关------------------------------------------
    virtual intrusive_ptr<Shader> complie_shader_program(const std::string& vs_src, const std::string& ps_src) const override;
    virtual std::unique_ptr<ShaderIntrospector> create_shader_introspect(Shader* shader) const override{
        return std::make_unique<GLShaderIntrospector>(shader);
    }

    virtual intrusive_ptr<Material> create_material(UberShader* uber_shader) override{
        return intrusive_ptr<Material>{new GLMaterial{uber_shader}};
    }
    // drawcall
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                                     std::optional<int> stencil = std::nullopt) noexcept override;
    virtual void clear(bool color = true, bool depth = true, bool stencil = true) const noexcept override;
    virtual void draw(intrusive_ptr<Mesh> mesh) override;

    virtual void bind_rendertarget_screen() noexcept override;
    virtual void set_viewport(int32_t x, int32_t y, int32_t w, int32_t h) noexcept override;

};

} // namespace Graphics

} // namespace Goonya