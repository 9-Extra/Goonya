#include "RenderResource.h"

#include <FreeImage.h>
#include <glad/glad.h>

#include <array>

#include "function/graphics/opengl_utils.h"
#include "resource/resources.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Graphics {

RenderReousce resources; // Global

FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_color = true) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    if (is_color) {
        FreeImage_AdjustGamma(pImage, 1 / 2.2); // 对于颜色贴图，进行矫正
    }
    FreeImage_Unload(pImage_ori);

    return pImage;
}

void RenderReousce::clear() {
    meshes.clear();
    materials.clear();
    cubemaps.clear();
    textures.clear();
    pso_cache.drop();
    for (auto &de : deconstructors) {
        de();
    }
    deconstructors.clear();
}
void RenderReousce::add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path) {
    LOG_TRACE("Load shader: {}", key);
    pso_cache.shader_lib.add_uber_shader(key, {vs_path, ps_path});
}
void RenderReousce::add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
                                const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                                const std::string &image_nz) {
    LOG_TRACE("Load skybox: {}", key);

    // GL_TEXTURE_CUBE_MAP_POSITIVE_X
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    std::array<const std::string *, 6> textures_faces{
        &image_px, &image_nx, &image_py, &image_ny, &image_pz, &image_nz,
    };

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

    cubemaps.add(key, CubeMap{texture_id});

    deconstructors.emplace_back([texture_id] { glDeleteTextures(1, &texture_id); });
}
void RenderReousce::add_texture(const std::string &key, const std::string &image_path, bool is_color) {
    LOG_TRACE("Load texture: {}", key);

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

    textures.add(key, Texture{texture_id});

    deconstructors.emplace_back([texture_id]() { glDeleteTextures(1, &texture_id); });
}

void RenderReousce::add_material(const std::string &key, const Resource::MaterialDesc &desc) {
    LOG_TRACE("Load material: {}", key);
    Material mat;
    mat.pipeline_state = pso_cache.query_pso(desc.pso_desc);

    for (const auto &u : desc.uniforms) {
        unsigned int buffer_id;
        glGenBuffers(1, &buffer_id);
        glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
        glBufferData(GL_UNIFORM_BUFFER, u.size, u.data, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        mat.uniforms.emplace_back(Material::UniformData{u.binding_id, buffer_id});

        deconstructors.emplace_back([buffer_id]() { glDeleteBuffers(1, &buffer_id); });
    }
    for (const auto &s : desc.samplers) {
        GLuint texture_id;
        GLenum texture_type;
        GLenum warp_mode;
        GLenum min_filter, mag_filter;
        // 纹理类型
        if (s.texture_type == "rgb") {
            texture_id = textures.get(textures.find(s.texture_key)).texture_id;
            texture_type = GL_TEXTURE_2D;
        } else if (s.texture_type == "cubemap") {
            texture_id = cubemaps.get(cubemaps.find(s.texture_key)).texture_id;
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

        mat.samplers.emplace_back(
            Material::SampleData{s.binding_id, texture_id, texture_type, min_filter, mag_filter, warp_mode});
    }

    materials.add(key, std::move(mat));
}
void RenderReousce::add_mesh(const std::string &key, const Vertex *vertices, size_t vertex_count,
                             const uint16_t *const indices, size_t indices_count) {
    LOG_TRACE("Load mesh: {}", key);
    unsigned int vao_id, ibo_id, vbo_id;
    glGenVertexArrays(1, &vao_id);
    glGenBuffers(1, &ibo_id);
    glGenBuffers(1, &vbo_id);

    glBindVertexArray(vao_id);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(uint16_t), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    checkError();

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    meshes.add(key, Mesh{vao_id, (uint32_t)indices_count});

    deconstructors.emplace_back([vao_id, vbo_id, ibo_id]() {
        glDeleteVertexArrays(1, &vao_id);
        glDeleteBuffers(1, &vbo_id);
        glDeleteBuffers(1, &ibo_id);
        checkError();
    });
}

void RenderReousce::bind_material(const Material &mat) const {
    pso_cache.bind_pipeline_object(mat.pipeline_state); // 绑定此材质关联的着色器

    // 绑定材质的uniform buffer
    for (const Material::UniformData &u : mat.uniforms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, u.binding_id, u.buffer_id);
    }
    // 绑定所有纹理
    for (const Material::SampleData &s : mat.samplers) {
        glActiveTexture(GL_TEXTURE0 + s.binding_id);
        glTextureParameteri(s.texture_id, GL_TEXTURE_MIN_FILTER, s.min_filter);
        glTextureParameteri(s.texture_id, GL_TEXTURE_MAG_FILTER, s.mag_filter);
        glTextureParameteri(s.texture_id, GL_TEXTURE_WRAP_R, s.warp_mode);
        glTextureParameteri(s.texture_id, GL_TEXTURE_WRAP_S, s.warp_mode);
        glTextureParameteri(s.texture_id, GL_TEXTURE_WRAP_T, s.warp_mode);

        glBindTexture(s.texture_type, s.texture_id);
    }
    checkError();
}

} // namespace Graphics
} // namespace Goonya
