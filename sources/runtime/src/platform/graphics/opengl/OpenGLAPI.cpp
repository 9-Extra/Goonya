#include "OpenGLAPI.h"
#include "core/log/Log.h"
#include <FreeImage.h> // FreeImage不知为何定义了_WINDOWS_，导致spdlog包含的Windows.h头文件不完整，所以先包含spdlog
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdint>
#include <glad/glad.h>
#include <nowide/convert.hpp>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>

#include "GLBuffer.h"
#include "GLRenderTarget.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLShader.h"

namespace Goonya {
namespace Graphics {

OpenGLGraphicsAPI::OpenGLGraphicsAPI() {
    // 初始化日志
    {
        const auto sinks = Logger::get_sinks();
        logger = std::make_shared<spdlog::async_logger>("OpenGL", sinks.begin(), sinks.end(), spdlog::thread_pool());
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
    }
    
    // 加载OpenGL函数
    {
        GLenum err = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
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
#ifdef NDEBUG
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
    }
}

// -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
intrusive_ptr<Mesh> OpenGLGraphicsAPI::load_mesh(const MeshDesc &desc) {
    GLuint vao_id;
    glCreateVertexArrays(1, &vao_id);
    // Goonya定义的uv以右上角为原点，而OpenGL的uv使用左下角
    intrusive_ptr<GLBuffer> vertex_buffer{desc.raw_vertices.as_span<uint8_t>(), BufferType::STATIC};
    intrusive_ptr<GLBuffer> index_buffer{std::span(desc.indices), BufferType::STATIC};

    return intrusive_ptr<GLMesh>{new GLMesh{desc.sub_meshes, desc.vertex_layout, vertex_buffer, index_buffer}};
}

intrusive_ptr<Buffer> OpenGLGraphicsAPI::create_buffer(uint32_t size, BufferType type) {
    return intrusive_ptr<GLBuffer>(new GLBuffer(size, type));
}

intrusive_ptr<FrameBuffer> OpenGLGraphicsAPI::create_rendertarget(std::tuple<uint32_t, uint32_t> size) {
    return intrusive_ptr<GLFrameBuffer>(size);
}

intrusive_ptr<Shader> OpenGLGraphicsAPI::complie_shader_program(const std::string &vs_src,
                                                                const std::string &ps_src) const {
    return intrusive_ptr<GLShader>{vs_src, ps_src};
}

// ---------------------------------绘制调用-------------------------------------------------------------
void OpenGLGraphicsAPI::set_clear_parameter(std::optional<Color> color, std::optional<float> depth,
                                            std::optional<int> stencil) noexcept {
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
    GLbitfield bit = 0;
    bit |= color ? GL_COLOR_BUFFER_BIT : 0;
    bit |= depth ? GL_DEPTH_BUFFER_BIT : 0;
    bit |= stencil ? GL_STENCIL_BUFFER_BIT : 0;
    glClear(bit);
}

void OpenGLGraphicsAPI::draw(intrusive_ptr<Mesh> mesh) {
    GLMesh *gl_mesh = dynamic_cast<GLMesh *>(mesh.get());
    assert(gl_mesh);

    gl_mesh->bind();
    for (const GLSubMesh &submesh : gl_mesh->submeshes) {
        glDrawElements(submesh.topology, submesh.index_count, GL_UNSIGNED_SHORT,
                       reinterpret_cast<void *>(submesh.start_index)); // 绘制
    }
}

// -----------------------bind-------------------------------
void OpenGLGraphicsAPI::set_viewport(const Viewport &view_port) noexcept {
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
} // namespace Graphics
} // namespace Goonya