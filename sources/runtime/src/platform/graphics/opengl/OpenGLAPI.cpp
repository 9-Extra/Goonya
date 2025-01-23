#include "OpenGLAPI.h"
#include <FreeImage.h>
#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <ranges>

#include "GLBuffer.h"
#include "GLResource.h"
#include "GLTexture.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/GraphicsResource.h"
#include "platform/graphics/graphics.h"
#include "runtime/GoonyaException.h"
#include "runtime/log/Log.h"

namespace Goonya {
namespace Graphics {

static FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_color = true) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
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
    GLenum err = gladLoadGL();
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

    check_error(__FILE__, __LINE__); // 现在graphics指针还没有设置，不能用宏
}

void OpenGLGraphicsAPI::check_error(const char *file, size_t line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR("GL error 0x{}: At: {}:{}", error, file, line);
    }
}

intrusive_ptr<PipelineStateObject> OpenGLGraphicsAPI::query_pso(const Resource::PSODesc &desc) {
    return pso_cache.query_pso(desc);
}
// -------------加载资源到设备，仅包括最底层的资源，高级别的资源由Renderer负责------------------
void OpenGLGraphicsAPI::load_uber_shader(const std::string &name, const UberShaderDesc &desc) {
    pso_cache.add_uber_shader(name, desc);
}

intrusive_ptr<Material> OpenGLGraphicsAPI::load_material(const Resource::MaterialDesc &desc,
                                                         const std::vector<intrusive_ptr<Texture>> &textures) {
    intrusive_ptr<GLMaterial> mat{new GLMaterial()};
    mat->pipeline_state = pso_cache.query_pso(desc.pso_desc);

    for (const auto &u : desc.uniforms) {
        GLuint buffer_id;
        glCreateBuffers(1, &buffer_id);
        glNamedBufferData(buffer_id, u.size, u.data, GL_STATIC_DRAW);
        mat->uniforms.emplace_back(GLMaterial::UniformData{u.binding_id, buffer_id});
    }

    for (const auto &[s, t] : std::views::zip(desc.samplers, textures)) {
        GLenum texture_type;
        GLenum warp_mode;
        GLenum min_filter, mag_filter;
        // 纹理类型
        if (s.texture_type == "rgb") {
            texture_type = GL_TEXTURE_2D;
        } else if (s.texture_type == "cubemap") {
            texture_type = GL_TEXTURE_CUBE_MAP;
        } else {
            throw Goonya::RuntimeError(std::format("无效的纹理类型：{}", s.texture_type));
        }
        // 重复模式
        if (s.warp_mode == "repeat") {
            warp_mode = GL_REPEAT;
        } else if (s.warp_mode == "clamp") {
            warp_mode = GL_CLAMP_TO_EDGE;
        } else if (s.warp_mode == "mirror") {
            warp_mode = GL_MIRRORED_REPEAT;
        } else {
            throw Goonya::RuntimeError(std::format("无效的warp_mode：{}", s.warp_mode));
        }
        // 缩放（采样）模式
        if (s.filter_mode == "point") {
            min_filter = GL_NEAREST;
            mag_filter = GL_NEAREST;
        } else if (s.filter_mode == "bilinear") {
            min_filter = GL_LINEAR;
            mag_filter = GL_LINEAR;
        } else if (s.filter_mode == "trilinear") {
            min_filter = GL_LINEAR_MIPMAP_LINEAR;
            mag_filter = GL_LINEAR;
        } else {
            throw Goonya::RuntimeError(std::format("无效的filter_mode：{}", s.filter_mode));
        }

        mat->samplers.emplace_back(
            GLMaterial::SampleData{s.binding_id, t, texture_type, min_filter, mag_filter, warp_mode});
    }

    return mat;
}

intrusive_ptr<Mesh> OpenGLGraphicsAPI::load_mesh(const VertexLayout& vertex_layout, std::span<const uint8_t> raw_vertices, std::span<const uint16_t> indices) {
    GLuint vao_id;
    glCreateVertexArrays(1, &vao_id);

    intrusive_ptr<GLVertexBuffer> vertex_buffer{vertex_layout, raw_vertices};
    intrusive_ptr<GLIndexBuffer> index_buffer{indices};
    
    {
        // 绑定
        glBindVertexArray(vao_id);
        vertex_buffer->bind_vertices();
        index_buffer->bind_indices();
        glBindVertexArray(0);
        checkError();
    }

    return intrusive_ptr<GLMesh>{new GLMesh{vao_id, vertex_buffer, index_buffer}};
}
// virtual void load_material(const std::string &key, const Resource::MaterialDesc &desc);
intrusive_ptr<Texture2D> OpenGLGraphicsAPI::load_texture2D(const std::string &image_path, bool is_color) {

    FIBITMAP *pImage = freeimage_load_and_convert_image(image_path, is_color);

    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Parameter 现在由材质决定
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
                 (void *)FreeImage_GetBits(pImage));
    glGenerateTextureMipmap(texture_id);

    checkError();

    FreeImage_Unload(pImage);

    glBindTexture(GL_TEXTURE_2D, 0);

    return intrusive_ptr<GLTexture2D>(new GLTexture2D{texture_id, nWidth, nHeight});
}
intrusive_ptr<TextureCube> OpenGLGraphicsAPI::load_cubemap(const std::string &image_px, const std::string &image_nx,
                                                           const std::string &image_py, const std::string &image_ny,
                                                           const std::string &image_pz, const std::string &image_nz) {

    // GL_TEXTURE_CUBE_MAP_POSITIVE_X
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    std::array<const std::string *, 6> textures_faces{
        &image_px, &image_nx, &image_py, &image_ny, &image_pz, &image_nz,
    };

    GLuint texture_id;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);

    // glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    for (unsigned int i = 0; i < textures_faces.size(); i++) {
        FIBITMAP *pImage = freeimage_load_and_convert_image(*textures_faces[i]);

        unsigned int nWidth = FreeImage_GetWidth(pImage);
        unsigned int nHeight = FreeImage_GetHeight(pImage);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
                     (void *)FreeImage_GetBits(pImage));

        FreeImage_Unload(pImage);
    }
    glGenerateTextureMipmap(texture_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    checkError();
    
    return intrusive_ptr<GLTextureCube>(new GLTextureCube{texture_id});
}

intrusive_ptr<Buffer> OpenGLGraphicsAPI::create_buffer(uint32_t size, BufferType type) {
    return intrusive_ptr<GLBuffer>(new GLBuffer(size, type));
}

intrusive_ptr<UniformBuffer> OpenGLGraphicsAPI::create_uniform_buffer(uint32_t size, BufferType type) {
    return intrusive_ptr<GLUniformBuffer>(size, type);
}

} // namespace Graphics
} // namespace Goonya