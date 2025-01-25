#pragma once

#include "../graphics.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/GraphicsResource.h"
#include "shaderlib/pso_cache.h"

namespace Goonya {
namespace Graphics {

class OpenGLGraphicsAPI : public GraphicsAPI {
public:
    OpenGLGraphicsAPI();
    ~OpenGLGraphicsAPI() {
        // nothing
    }

    virtual void check_error(const char* file, size_t line) override;
    virtual intrusive_ptr<PipelineStateObject> query_pso(const Resource::PSODesc &desc) override;

    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
    virtual void load_uber_shader(const std::string &name, const UberShaderDesc &desc) override;
    virtual intrusive_ptr<Mesh> load_mesh(Topology topology, const VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) override;
    virtual intrusive_ptr<Material> load_material(const Resource::MaterialDesc &desc, const std::vector<intrusive_ptr<Texture>>& textures) override;
    virtual intrusive_ptr<Texture2D> load_texture2D(const std::string &image_path, bool is_color = false) override;
    virtual intrusive_ptr<TextureCube> load_cubemap(const std::string &image_px, const std::string &image_nx,
                     const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                     const std::string &image_nz) override;

    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) override;
    virtual intrusive_ptr<UniformBuffer> create_uniform_buffer(uint32_t size, BufferType type) override;

    virtual intrusive_ptr<RenderTarget> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) override;
    // drawcall
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                                     std::optional<int> stencil = std::nullopt) noexcept override;
    virtual void clear(bool color = true, bool depth = true, bool stencil = true) const noexcept override;
    virtual void draw(intrusive_ptr<Mesh> mesh) override;

    virtual void bind_rendertarget_screen() noexcept override;
    virtual void set_viewport(int32_t x, int32_t y, int32_t w, int32_t h) noexcept override;
private:
    PSOCache pso_cache;

};

} // namespace Graphics

} // namespace Goonya