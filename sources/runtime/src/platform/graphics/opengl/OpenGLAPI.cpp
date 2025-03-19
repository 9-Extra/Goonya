#include "OpenGLAPI.h"
#include "core/log/Log.h"
#include <FreeImage.h> // FreeImage不知为何定义了_WINDOWS_，导致spdlog包含的Windows.h头文件不完整，所以先包含spdlog
#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <glad/glad.h>

#include "GLBuffer.h"
#include "GLMaterial.h"
#include "GLTexture.h"
#include "GLRenderTarget.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/resources.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

static FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_color) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        throw RuntimeError(std::format("Failed to load image: {}", image_path));
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    if (is_color) {
        // 对于颜色贴图，进行矫正
        FreeImage_AdjustGamma(pImage, 1 / 2.2); // FreeImage的实现中用1/gamme，所以这里的1/2.2是对的
    }
    FreeImage_Unload(pImage_ori);

    return pImage;
}

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

    opengl_debug_check_error();
}

intrusive_ptr<PipelineStateObject> OpenGLGraphicsAPI::query_pso(const Resource::PSODesc &desc) {
    return pso_cache.query_pso(desc);
}
// -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
void OpenGLGraphicsAPI::load_uber_shader(const std::string &name, const UberShaderDesc &desc) {
    pso_cache.add_uber_shader(name, desc);
}

intrusive_ptr<Mesh> OpenGLGraphicsAPI::load_mesh(Topology topology, const Resource::VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) {
    GLuint vao_id;
    glCreateVertexArrays(1, &vao_id);

    intrusive_ptr<GLBuffer> vertex_buffer{raw_vertices, BufferType::STATIC};
    intrusive_ptr<GLBuffer> index_buffer{indices, BufferType::STATIC};

    return intrusive_ptr<GLMesh>{new GLMesh{topology, vertex_layout, vertex_buffer, index_buffer}};
}
// virtual void load_material(const std::string &key, const Resource::MaterialDesc &desc);
intrusive_ptr<Texture> OpenGLGraphicsAPI::load_texture2D(const Resource::Texture2DDesc& desc) const {
    FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path, desc.is_srgb);

    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);
    
    intrusive_ptr<GLTexture> texture{new GLTexture(TextureType::TEXTURE_2D, {nWidth, nHeight, 0})};
    texture->set_filter_mode(desc.filter_mode);
    texture->set_warp_mode(desc.warp_mode);

    glTextureSubImage2D(texture->get_id(), 0, 0, 0, nWidth, nHeight, GL_BGR, GL_UNSIGNED_BYTE, (void *)FreeImage_GetBits(pImage));
    texture->generate_mipmaps();

    opengl_debug_check_error();

    FreeImage_Unload(pImage);

    return texture;
}
intrusive_ptr<Texture> OpenGLGraphicsAPI::load_cubemap(const Resource::TextureCubeMapDesc& desc) const {
    // 使用第一张图像的宽高信息分配纹理空间
    FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path[0], desc.is_srgb);
    
    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    intrusive_ptr<GLTexture> texture{new GLTexture(TextureType::TEXTURE_CUBEMAP, {nWidth, nHeight, 0})};
    texture->set_filter_mode(desc.filter_mode);
    texture->set_warp_mode(desc.warp_mode);

    // CubeMap可以使用3D纹理的加载函数进行加载，使用zoffset参数制定加载的图像的方向
    glTextureSubImage3D(texture->get_id(), 0, 0, 0, 0, nWidth, nHeight, 1, GL_BGR, GL_UNSIGNED_BYTE, (void *)FreeImage_GetBits(pImage));      
    FreeImage_Unload(pImage);

    // 加载其余方向上的图像
    for (unsigned int i = 1; i < desc.path.size(); i++) {
        FIBITMAP *pImage = freeimage_load_and_convert_image(desc.path[i], desc.is_srgb);    
        if (nWidth != FreeImage_GetWidth(pImage) || nHeight != FreeImage_GetHeight(pImage)){
            throw RuntimeError(std::format("CubeMap{}的大小不一致", desc.path[i]));
        }
        glTextureSubImage3D(texture->get_id(), 0, 0, 0, i, nWidth, nHeight, 1, GL_BGR, GL_UNSIGNED_BYTE, (void *)FreeImage_GetBits(pImage));      
        FreeImage_Unload(pImage);
    }
    texture->generate_mipmaps();

    opengl_debug_check_error();
    
    return texture;
}

intrusive_ptr<Buffer> OpenGLGraphicsAPI::create_buffer(uint32_t size, BufferType type) {
    return intrusive_ptr<GLBuffer>(new GLBuffer(size, type));
}

intrusive_ptr<RenderTarget> OpenGLGraphicsAPI::create_rendertarget(std::tuple<uint32_t, uint32_t> size) {
    return intrusive_ptr<GLRenderTarget>(size);
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
    GLMesh* gl_mesh = dynamic_cast<GLMesh*>(mesh.get());
    assert(gl_mesh);

    gl_mesh->bind();
    for(const GLSubMesh& submesh : gl_mesh->submeshes){
        glDrawElements(submesh.topology, submesh.index_count, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(submesh.start_index)); // 绘制
    }

    check_error();
}

// -----------------------bind-------------------------------
void OpenGLGraphicsAPI::bind_rendertarget_screen() noexcept{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // 绑定默认帧缓冲
    glDrawBuffer(GL_BACK); // 渲染到后缓冲区
}
void OpenGLGraphicsAPI::set_viewport(int32_t x, int32_t y, int32_t w, int32_t h) noexcept{
    glViewport(x, y, w, h);
}
} // namespace Graphics
} // namespace Goonya