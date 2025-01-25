#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sys/types.h>
#include <vector>

#include "GraphicsResource.h"
#include "Buffer.h"
#include "Texture.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/RenderTarget.h"
#include "resource/resources.h"

#ifdef NDEBUG
#define checkError()
#else
#define checkError() ::Goonya::Graphics::graphics_api->check_error(__FILE__, __LINE__)
#endif // !NDEBUG

namespace Goonya {
namespace Graphics {

enum class GraphicsAPIType{
    NONE,
    OPENGL,
};

class GraphicsAPI{
public:
    GraphicsAPI() = default;
    virtual ~GraphicsAPI() = default;

    virtual void check_error(const char* file, size_t line) = 0;
    virtual intrusive_ptr<PipelineStateObject> query_pso(const Resource::PSODesc &desc) = 0;

    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
    virtual void load_uber_shader(const std::string &name, const UberShaderDesc &desc) = 0;
    virtual intrusive_ptr<Mesh> load_mesh(Topology topology, const VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) = 0;
    virtual intrusive_ptr<Material> load_material(const Resource::MaterialDesc &desc, const std::vector<intrusive_ptr<Texture>>& textures) = 0;
    virtual intrusive_ptr<Texture2D> load_texture2D(const std::string &image_path, bool is_color = false) = 0;
    virtual intrusive_ptr<TextureCube> load_cubemap(const std::string &image_px, const std::string &image_nx,
                     const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                     const std::string &image_nz) = 0;
    
    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) = 0;
    virtual intrusive_ptr<UniformBuffer> create_uniform_buffer(uint32_t size, BufferType type) = 0;

    virtual intrusive_ptr<RenderTarget> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) = 0;
    // drawcall
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt, std::optional<int> stencil = std::nullopt) noexcept = 0;
    virtual void clear(bool color = true, bool depth = true, bool stencil = false) const noexcept = 0; // 清理当前绑定的RenderTarget
    virtual void draw(intrusive_ptr<Mesh> mesh) = 0;

    // bind
    // virtual void bind_render_target() = 0;
    virtual void bind_rendertarget_screen() noexcept = 0;
    virtual void set_viewport(int32_t x, int32_t y, int32_t w, int32_t h) noexcept = 0;
};

extern std::unique_ptr<GraphicsAPI> graphics_api;

void initialize(GraphicsAPIType api_type);

void drop();

}
}