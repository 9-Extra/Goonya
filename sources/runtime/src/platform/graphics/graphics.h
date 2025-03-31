#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>

#include "Buffer.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/RenderTarget.h"
#include "platform/graphics/Shader.h"

#define check_error() Goonya::Graphics::graphics_api->_check_error(__FILE__, __LINE__)
#ifdef NDEBUG
#define debug_check_error()
#else
#define debug_check_error() Goonya::Graphics::graphics_api->_check_error(__FILE__, __LINE__)
#endif // !NDEBUG

namespace Goonya {
namespace Graphics {

enum class GraphicsAPIType {
    NONE,
    OPENGL,
};

class GraphicsAPI {
public:
    GraphicsAPI() = default;
    virtual ~GraphicsAPI() = default;

    virtual void _check_error(const char *file, size_t line) = 0;
    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
    virtual intrusive_ptr<Mesh> load_mesh(Topology topology, const VertexLayout &vertex_layout,
                                          std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) = 0;
    virtual intrusive_ptr<Texture> create_texture(const TextureCreateDesc &desc) const = 0;

    virtual intrusive_ptr<Buffer> create_buffer(uint32_t size, BufferType type) = 0;

    virtual intrusive_ptr<FrameBuffer> create_rendertarget(std::tuple<uint32_t, uint32_t> size = {0, 0}) = 0;

    // ---------------------------------材质和着色器相关------------------------------------------
    virtual intrusive_ptr<Shader> complie_shader_program(const std::string &vs_src,
                                                         const std::string &ps_src) const = 0;
    virtual std::unique_ptr<ShaderIntrospector> create_shader_introspect(Shader *shader) const = 0;

    virtual intrusive_ptr<Material> create_material(UberShader *uber_shader) = 0;

    // ---------------------------------绘制调用-------------------------------------------------------------
    virtual intrusive_ptr<RenderTarget> get_rendertarget_screen() noexcept = 0;
    virtual void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                                     std::optional<int> stencil = std::nullopt) noexcept = 0;
    virtual void clear(bool color = true, bool depth = true,
                       bool stencil = false) const noexcept = 0; // 清理当前绑定的RenderTarget
    virtual void draw(intrusive_ptr<Mesh> mesh) = 0;

    // bind
    virtual void set_viewport(const Viewport &view_port) noexcept = 0;

    // --------------------其他------------------------------
    /**
     * @brief 计算透视投影矩阵
     *
     * Goonya的世界坐标系使用右手坐标系，即+X向右，+Y向上，+Z向前，相机在没有旋转的情况下朝向-Z方向。
     * 由此可知Goonya的相机坐标系中相机位于原点且朝向-Z方向，所有可见物体位于Z负半轴中以Z轴为中心的平截头体范围内，其近平面为
     * z = -near_z，远平面为 z = -far_z 透视投影矩阵将此平截头体变换到一个统一的标准设备空间，其为从(-1,-1, -1)到(1, 1,
     * 1)的小空间，投影在外的物理会被切除 相机在此空间中朝向+Z方向（本质上是因为Z重映射(默认到[0,
     * 1])成为了深度，而深度越大视为离相机越远），因此透视投影矩阵中包含了Z坐标取反
     *
     * @attention 这里的NDC与图形API的NDC定义不一致，需要使用GraphicsAPI::native_projection_matrix进行修补
     * @param ratio 视口的宽高比(w / h)
     * @param fov 视场角（垂直），平截头体上下平面的夹角，弧度制
     * @param near_z 近平面距离
     * @param far_z 远平面距离
     * @return Matrix4 透视投影矩阵
     */
    virtual Matrix4 compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                               bool render_to_texture = false) const noexcept = 0;
};

extern std::unique_ptr<GraphicsAPI> graphics_api;

void initialize(GraphicsAPIType api_type);

void drop();

} // namespace Graphics
} // namespace Goonya