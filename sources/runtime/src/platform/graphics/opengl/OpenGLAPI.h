#pragma once

#include "platform/graphics/PipelineSetting.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLRenderTarget.h"
#include "spdlog/logger.h"

#include <cstddef>
#include <cstdint>

namespace Goonya {

using Goonya::Color;

class OpenGLGraphicsAPI final {
public:
    std::shared_ptr<spdlog::logger> logger;

    void initialize();
    void drop() noexcept;
    bool is_initialized() const noexcept { return initialized; };

    // ---------------------------调试-----------------------------
    //  NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void push_debug_group_label(const std::string &label) const noexcept {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, (GLsizei)label.size(), label.data());
    };
    //  NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void pop_debug_group_label() const noexcept { glPopDebugGroup(); }

    // ---------------------------------材质和着色器相关------------------------------------------
    void set_pipeline_state(const PipelineSetting &state) const noexcept;

    // ---------------------------------绘制调用-------------------------------------------------------------
    Ref<RenderTarget> get_rendertarget_screen() noexcept { return rendertarget_screen; };
    void set_clear_parameter(std::optional<Color> color, std::optional<float> depth = std::nullopt,
                             std::optional<int> stencil = std::nullopt) noexcept;
    void clear(bool color, bool depth, bool stencil) const noexcept;
    void draw_submesh(const SubMesh &submesh) const;
    void draw_multidraw(Topology topology, int32_t *count_array, size_t *index_offset_array, int32_t *base_vertex_array,
                        int32_t count) const;

    void set_viewport(const Viewport &view_port) noexcept;

    // --------------------其他------------------------------
    Matrix4f compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                        bool render_to_texture) const noexcept;

private:
    Ref<RenderTarget> rendertarget_screen;
    bool initialized = false;
};

} // namespace Goonya
