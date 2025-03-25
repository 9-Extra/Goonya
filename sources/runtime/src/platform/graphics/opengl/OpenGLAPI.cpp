#include "OpenGLAPI.h"
#include "core/log/Log.h"
#include <FreeImage.h> // FreeImage不知为何定义了_WINDOWS_，导致spdlog包含的Windows.h头文件不完整，所以先包含spdlog
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdint>
#include <glad/glad.h>
#include <nowide/convert.hpp>
#include <string>

#include "GLBuffer.h"
#include "GLRenderTarget.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLShader.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

OpenGLGraphicsAPI::OpenGLGraphicsAPI() {
    GLenum err = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (err != GL_TRUE) {
        LOG_ERROR("gladLoadGL Error: {}", err);
    }

    const char *vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    LOG_INFO("vendor: {}, version: {}", vendorName, version);

    // if (!GL_EXT_gpu_shader4) {
    //     std::cerr << "不兼容拓展" << std::endl;
    // }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glFrontFace(GL_CCW); // 逆时针为正面
    /*
     * 启动无缝立方体贴图，允许硬件在立方体贴图边界“相邻”的纹理上跨界采样
     * 立方体贴图的warp_mode将被无视，参考https://registry.khronos.org/OpenGL/extensions/ARB/ARB_seamless_cube_map.txt
     */
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    opengl_check_error();
}

// -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
intrusive_ptr<Mesh> OpenGLGraphicsAPI::load_mesh(Topology topology, const VertexLayout &vertex_layout,
                                                 std::span<const uint8_t> raw_vertices,
                                                 std::span<const uint16_t> indices) {
    GLuint vao_id;
    glCreateVertexArrays(1, &vao_id);
    // Goonya定义的uv以右上角为原点，而OpenGL的uv使用左下角
    intrusive_ptr<GLBuffer> vertex_buffer{raw_vertices, BufferType::STATIC};
    intrusive_ptr<GLBuffer> index_buffer{indices, BufferType::STATIC};

    return intrusive_ptr<GLMesh>{new GLMesh{topology, vertex_layout, vertex_buffer, index_buffer}};
}

intrusive_ptr<Buffer> OpenGLGraphicsAPI::create_buffer(uint32_t size, BufferType type) {
    return intrusive_ptr<GLBuffer>(new GLBuffer(size, type));
}

intrusive_ptr<RenderTarget> OpenGLGraphicsAPI::create_rendertarget(std::tuple<uint32_t, uint32_t> size) {
    return intrusive_ptr<GLRenderTarget>(size);
}

unsigned int complie_shader(const std::string &source, unsigned int shader_type) {
    unsigned int id = glCreateShader(shader_type);

    const GLchar *data = source.c_str();
    GLint length = (GLint)source.length();
    glShaderSource(id, 1, &data, &length);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        throw RuntimeError(std::format("着色器编译错误： {}", infoLog));
    }

    return id;
}
intrusive_ptr<Shader> OpenGLGraphicsAPI::complie_shader_program(const std::string &vs_src,
                                                                const std::string &ps_src) const {
    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    GLuint id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, ps);
    glLinkProgram(id);

    int success;
    char infoLog[512];

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        throw RuntimeError(std::format("着色器链接错误： {}", infoLog));
    }

    glDeleteShader(vs);
    glDeleteShader(ps);

    return intrusive_ptr<GLShader>{id};
}

// ---------------------drawcall---------------------------
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

    check_error();
}

// -----------------------bind-------------------------------
void OpenGLGraphicsAPI::bind_rendertarget_screen() noexcept {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // 绑定默认帧缓冲
    glDrawBuffer(GL_BACK);                     // 渲染到后缓冲区
}
void OpenGLGraphicsAPI::set_viewport(int32_t x, int32_t y, int32_t w, int32_t h) noexcept { glViewport(x, y, w, h); }
} // namespace Graphics
} // namespace Goonya