#include "resource/BuiltinResource.h"

#include "core/RefCount.h"
#include "function/renderer/Mesh.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "platform/image/image.h"
#include "resource/ResMng.h"
#include "resource/Resource.h"

namespace Goonya {

void init_buildin_resource() {
    Ref<ResourcePack> buildin = create_ref<ResourcePack>();
    resources.put_resource("buildin", buildin);
    { // 部分硬编码的mesh — plane
        MeshDataArrays mesh_data{.position =
                                     {
                                         {-1.0f, 1.0f, 0.0f},
                                         {1.0f, 1.0f, 0.0f},
                                         {1.0f, -1.0f, 0.0f},
                                         {-1.0f, -1.0f, 0.0f},
                                     },
                                 .normal =
                                     {
                                         {0.0f, 0.0f, 1.0f},
                                         {0.0f, 0.0f, 1.0f},
                                         {0.0f, 0.0f, 1.0f},
                                         {0.0f, 0.0f, 1.0f},
                                     },
                                 .tangent =
                                     {
                                         {1.0f, 0.0f, 0.0f, 1.0f},
                                         {1.0f, 0.0f, 0.0f, 1.0f},
                                         {1.0f, 0.0f, 0.0f, 1.0f},
                                         {1.0f, 0.0f, 0.0f, 1.0f},
                                     },
                                 .uv =
                                     {
                                         {0.0f, 0.0f},
                                         {1.0f, 0.0f},
                                         {1.0f, 1.0f},
                                         {0.0f, 1.0f},
                                     },
                                 .indices = {0, 1, 2, 2, 3, 0},
                                 .submeshes = {{SubMesh{.start_index = 0,
                                                        .index_count = 6,
                                                        .base_vertex_offset = 0,
                                                        .topology = Topology::TRIANGLE,
                                                        .aabb = {{-1.0f, -1.0f, -0.0001f}, {1.0f, 1.0f, 0.0001f}}}}}};

        buildin->contents.emplace("plane", create_ref<Mesh>(mesh_data));
    }

    { // 添加天空盒的mesh，因为只有位置所以格式不一样
        MeshDataArrays mesh_data;
        mesh_data.position = {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0}, {-1.0, 1.0, -1.0},
                              {-1.0, -1.0, 1.0},  {1.0, -1.0, 1.0},  {1.0, 1.0, 1.0},  {-1.0, 1.0, 1.0}};
        mesh_data.indices = {1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
                             2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};
        mesh_data.submeshes = {SubMesh{.start_index = 0,
                                       .index_count = 36,
                                       .base_vertex_offset = 0,
                                       .topology = Topology::TRIANGLE,
                                       .aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}}}};
        buildin->contents.emplace("skybox_cube", create_ref<Mesh>(mesh_data));
    }

    const uint32_t default_texture_size = 16;
    stb::Image image = stb::Image::create_empty(default_texture_size, default_texture_size, 3, false);
    struct Color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    {
        Ref<GLTexture> white = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16,
                                                     std::make_tuple(default_texture_size, default_texture_size, 0));
        for (size_t i = 0; i < image.get_size_byte(); i++) {
            ((uint8_t *)image.get_data())[i] = 255;
        }
        white->import_image(image);
        buildin->contents.emplace("white", white);
    }
    {
        Ref<GLTexture> black = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16,
                                                     std::make_tuple(default_texture_size, default_texture_size, 0));
        for (size_t i = 0; i < image.get_size_byte(); i++) {
            ((uint8_t *)image.get_data())[i] = 0;
        }
        black->import_image(image);
        buildin->contents.emplace("black", black);
    }
    {
        Ref<GLTexture> normal = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16,
                                                      std::make_tuple(default_texture_size, default_texture_size, 0));
        for (size_t i = 0; i < default_texture_size; i++) {
            for (size_t j = 0; j < default_texture_size; j++) {
                Color &color_ref = ((Color *)image.get_data())[i * default_texture_size + j];
                color_ref = {127, 127, 255}; // (0.5, 0.5, 1)
            }
        }
        normal->import_image(image);
        buildin->contents.emplace("normal", normal);
    }
    {
        Ref<GLTexture> missing = create_ref<GLTexture>(TextureType::TEXTURE_2D, TextureStorageFormat::RGB_f16,
                                                       std::make_tuple(default_texture_size, default_texture_size, 0));
        for (size_t i = 0; i < default_texture_size; i++) {
            for (size_t j = 0; j < default_texture_size; j++) {
                Color &color_ref = ((Color *)image.get_data())[i * default_texture_size + j];
                if ((i < default_texture_size / 2) != (j < default_texture_size / 2)) {
                    color_ref = {0, 0, 0}; // black
                } else {
                    color_ref = {128, 0, 128}; // purple
                }
            }
        }
        missing->import_image(image);
        buildin->contents.emplace("missing_texture", missing);
    }
}

} // namespace Goonya
