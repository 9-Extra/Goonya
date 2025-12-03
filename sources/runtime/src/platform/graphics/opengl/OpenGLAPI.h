#pragma once

#include "../Graphics.h"
#include "GLRenderTarget.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "platform/graphics/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Goonya::Graphics {

class OpenGLGraphicsAPI final : public GraphicsAPI {
public:
    OpenGLGraphicsAPI();
    ~OpenGLGraphicsAPI() override;

    // ---------------------------调试-----------------------------
    void push_debug_group_label(const std::string &label) const noexcept override {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, (GLsizei)label.size(), label.data());
    };
    void pop_debug_group_label() const noexcept override { glPopDebugGroup(); }

    // -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
    Ref<Mesh> create_mesh(VertexLayout layout) override;
    Ref<Texture> create_texture(const TextureCreateDesc &desc) const override {
        return create_ref<GLTexture>(desc);
    };
    Ref<Buffer> create_buffer(uint32_t size, BufferType type) override;
    Ref<FrameBuffer> create_rendertarget(std::tuple<uint32_t, uint32_t> size) override;

    // ---------------------------------材质和着色器相关------------------------------------------
    Ref<Shader> compile_shader_program(const std::string &vs_src, const std::string &ps_src) const override;
    std::unique_ptr<ShaderIntrospector> create_shader_introspect(Shader *shader) const override {
        return std::make_unique<GLShaderIntrospector>(shader);
    }

    void set_pipeline_state(const PipelineSetting &state) const noexcept override;

    // ---------------------------------绘制调用-------------------------------------------------------------
    Ref<RenderTarget> get_rendertarget_screen() noexcept override { return rendertarget_screen; };
    void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                             std::optional<int> stencil = std::nullopt) noexcept override;
    void clear(bool color, bool depth, bool stencil) const noexcept override;
    void draw_submesh(const SubMesh &submesh) const override;
    void draw_multidraw(Topology topology, int32_t* count_array, size_t* index_offset_array, int32_t* base_vertex_array, int32_t count) const override;

    void set_viewport(const Viewport &view_port) noexcept override;

    // --------------------其他------------------------------
    Matrix4 compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                       bool render_to_texture) const noexcept override;

private:
    Ref<RenderTarget> rendertarget_screen;
};

} // namespace Goonya::Graphics
