#include "OpenGLAPI.h"
#include "core/log/Log.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

#include "GLBuffer.h"
#include "GLRenderTarget.h"
#include "core/RefCount.h"
#include "function/renderer/RendererBasic.h"
#include "platform/display/display.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Mesh.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLShader.h"

namespace Goonya::Graphics {

OpenGLGraphicsAPI::OpenGLGraphicsAPI() {
    ASSERT_RENDER_THREAD();

    // 初始化日志
    {
        const auto &sinks = Logger::get_sinks();
        logger = std::make_shared<spdlog::async_logger>("OpenGL", sinks.begin(), sinks.end(), spdlog::thread_pool());
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
    }
    // 创建OpenGL上下文
    glfwMakeContextCurrent(Display::window);

    // 加载OpenGL函数
    {
        GLenum err = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
        if (err != GL_TRUE) {
            logger->error("gladLoadGL Error: {}", err);
        }
    }

    // 显示显卡驱动信息
    {
        const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
        const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        logger->info("vendor: {}, version: {}", vendorName, version);
    }

    // 注册消息回调
    {
        glDebugMessageCallback(_opengl_debug_callback, nullptr);
        glEnable(GL_DEBUG_OUTPUT); // 无论是否为调试模式，都要打开，不然不报错
        // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#ifndef DEBUG
        // 只报告重要的事
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // 非调试模式下关闭同步日志
#endif
    }

    // 其他
    {
        // OpenGL默认renderframe的封装
        rendertarget_screen = new GLRenderTargetScreen();

        glClearColor(0.0, 0.0, 0.0, 0.0);
        /*
         * 启动无缝立方体贴图，允许硬件在立方体贴图边界“相邻”的纹理上跨界采样
         * 立方体贴图的warp_mode将被无视，参考https://registry.khronos.org/OpenGL/extensions/ARB/ARB_seamless_cube_map.txt
         */
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        glfwSwapInterval(1); // 垂直同步
    }
}

OpenGLGraphicsAPI::~OpenGLGraphicsAPI() {
    ASSERT_RENDER_THREAD();
    // 取消消息回调防止logger在销毁后被使用
    glDebugMessageCallback(nullptr, nullptr); // 文档没写怎么反注册消息回调，猜是这样
    glfwMakeContextCurrent(nullptr);          // 清除OpenGL上下文
}

// -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
Ref<Mesh> OpenGLGraphicsAPI::create_mesh(VertexLayout layout) {
    ASSERT_RENDER_THREAD();
    return create_ref<GLMesh>(std::move(layout));
}

Ref<Buffer> OpenGLGraphicsAPI::create_buffer(uint32_t size, BufferType type) {
    ASSERT_RENDER_THREAD();
    return Ref<GLBuffer>(new GLBuffer(size, type));
}

Ref<FrameBuffer> OpenGLGraphicsAPI::create_rendertarget(std::tuple<uint32_t, uint32_t> size) {
    ASSERT_RENDER_THREAD();
    return create_ref<GLFrameBuffer>(size);
}

Ref<Shader> OpenGLGraphicsAPI::compile_shader_program(const std::string &vs_src, const std::string &ps_src) const {
    ASSERT_RENDER_THREAD();
    return create_ref<GLShader>(vs_src, ps_src);
}

static GLenum gl_blend_op(BlendOp op) noexcept {
    switch (op) {
    case BlendOp::ADD:
        return GL_FUNC_ADD;
    case BlendOp::SUB:
        return GL_FUNC_SUBTRACT;
    case BlendOp::REV_SUB:
        return GL_FUNC_REVERSE_SUBTRACT;
    case BlendOp::MIN:
        return GL_MIN;
    case BlendOp::MAX:
        return GL_MAX;
    }
    std::unreachable();
}

static GLenum gl_blend_param(BlendFactor factor) noexcept {
    switch (factor) {
    case BlendFactor::ZERO:
        return GL_ZERO;
    case BlendFactor::ONE:
        return GL_ONE;
    case BlendFactor::SRC_COLOR:
        return GL_SRC_COLOR;
    case BlendFactor::DST_COLOR:
        return GL_DST_COLOR;
    case BlendFactor::ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFactor::ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFactor::SRC_ALPHA:
        return GL_SRC_ALPHA;
    case BlendFactor::DST_ALPHA:
        return GL_DST_ALPHA;
    case BlendFactor::ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    }
    std::unreachable();
}

void OpenGLGraphicsAPI::set_pipeline_state(const PipelineSetting &state) const noexcept {
    ASSERT_RENDER_THREAD();
    // 深度测试
    bool enable_depth_test = true;
    GLenum depth_func = 0;

    switch (state.depth_test) {
    case DepthTestMode::LESS: {
        depth_func = GL_LESS;
        break;
    }
    case DepthTestMode::LESS_EQUAL: {
        depth_func = GL_LEQUAL;
        break;
    }
    case DepthTestMode::GREATER: {
        depth_func = GL_GREATER;
        break;
    }
    case DepthTestMode::GREATER_EQUAL: {
        depth_func = GL_GEQUAL;
        break;
    }
    case DepthTestMode::NEVER: {
        depth_func = GL_NEVER;
        break;
    }
    case DepthTestMode::ALWAYS: {
        depth_func = GL_ALWAYS;
        break;
    }
    case DepthTestMode::DISABLE: {
        enable_depth_test = false;
        break;
    }
    default:
        std::unreachable();
    }

    if (enable_depth_test) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depth_func);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    bool enable_cull_face = true;
    GLenum gl_cull_mode = 0;

    switch (state.cull_mode) {

    case CullFaceMode::BACK: {
        gl_cull_mode = GL_BACK;
        break;
    }
    case CullFaceMode::FRONT: {
        gl_cull_mode = GL_FRONT;
        break;
    }
    case CullFaceMode::FRONT_AND_BACK: {
        gl_cull_mode = GL_FRONT_AND_BACK;
        break;
    }
    case CullFaceMode::DISABLE: {
        enable_cull_face = false;
        break;
    default:
        std::unreachable();
    }
    }

    if (enable_cull_face) {
        glEnable(GL_CULL_FACE);
        glCullFace(gl_cull_mode);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if (state.is_blend_enable) {
        glEnable(GL_BLEND);

        GLenum color_op = gl_blend_op(state.blendop_color);
        GLenum alpha_op = gl_blend_op(state.blendop_alpha);

        GLenum src_color_factor = gl_blend_param(state.src_color_factor);
        GLenum dst_color_factor = gl_blend_param(state.dst_color_factor);
        GLenum src_alpha_factor = gl_blend_param(state.src_alpha_factor);
        GLenum dst_alpha_factor = gl_blend_param(state.dst_alpha_factor);

        glBlendEquationSeparate(color_op, alpha_op);
        glBlendFuncSeparate(src_color_factor, dst_color_factor, src_alpha_factor, dst_alpha_factor);
    } else {
        glDisable(GL_BLEND);
    }
};

// ---------------------------------绘制调用-------------------------------------------------------------
void OpenGLGraphicsAPI::set_clear_parameter(std::optional<Color> color, std::optional<float> depth,
                                            std::optional<int> stencil) noexcept {
    ASSERT_RENDER_THREAD();
    if (color) {
        Color c = *color;
        glClearColor(c.r, c.g, c.b, c.a);
    }
    if (depth) {
        glClearDepthf(*depth);
    }
    if (stencil) {
        glClearStencil(*stencil);
    }
};
void OpenGLGraphicsAPI::clear(bool color, bool depth, bool stencil) const noexcept {
    ASSERT_RENDER_THREAD();
    GLbitfield bit = 0;
    bit |= color ? GL_COLOR_BUFFER_BIT : 0;
    bit |= depth ? GL_DEPTH_BUFFER_BIT : 0;
    bit |= stencil ? GL_STENCIL_BUFFER_BIT : 0;
    glClear(bit);
}

static GLenum Topology2OpenGL(Topology t) noexcept {
    switch (t) {
    case Topology::POINT:
        return GL_POINTS;
    case Topology::LINE:
        return GL_LINES;
    case Topology::TRIANGLE:
        return GL_TRIANGLES;
    }
    return GL_INVALID_VALUE;
}

void OpenGLGraphicsAPI::draw_submesh(const SubMesh &submesh) const {
    ASSERT_RENDER_THREAD();
    glDrawElementsBaseVertex(Topology2OpenGL(submesh.topology), submesh.index_count, GL_UNSIGNED_INT,
                             reinterpret_cast<void *>((size_t)submesh.start_index * sizeof(uint32_t)),
                             submesh.base_vertex_offset); // 绘制
}

// -----------------------bind-------------------------------
void OpenGLGraphicsAPI::set_viewport(const Viewport &view_port) noexcept {
    ASSERT_RENDER_THREAD();
    glViewport(view_port.x, view_port.y, view_port.width, view_port.height);
}

Matrix4 OpenGLGraphicsAPI::compute_perspective_matrix(float ratio, float fov, float near_z, float far_z,
                                                      bool render_to_texture) const noexcept {
    assert(near_z < far_z); // 不要写反了！！！！！！！！！！
    float c = 1.0f / std::tan(fov / 2);

    if (render_to_texture) {
        // 翻转Y轴
        return Matrix4{c / ratio,
                       0.0f,
                       0.0f,
                       0.0f,
                       0.0f,
                       -c,
                       0.0f,
                       0.0f,
                       0.0f,
                       0.0f,
                       -(near_z + far_z) / (far_z - near_z),
                       -1.0f,
                       0.0f,
                       0.0f,
                       -2 * far_z * near_z / (far_z - near_z),
                       0.0f};
    } else {
        return Matrix4{c / ratio,
                       0.0f,
                       0.0f,
                       0.0f,
                       0.0f,
                       c,
                       0.0f,
                       0.0f,
                       0.0f,
                       0.0f,
                       -(near_z + far_z) / (far_z - near_z),
                       -1.0f,
                       0.0f,
                       0.0f,
                       -2 * far_z * near_z / (far_z - near_z),
                       0.0f};
    }
}
} // namespace Goonya::Graphics
