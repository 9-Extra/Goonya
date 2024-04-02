#include "RenderResource.h"

#include <glad/glad.h>
#include <freeimage/FreeImage.h>

#include <array>
#include <fstream>

#include "utils/cgmath.h"
#include <json/json.h>
#include "../utils.h"

std::string read_whole_file(const std::string &path);

RenderReousce resources; // Global

unsigned int complie_shader(const char *const src, unsigned int shader_type) {
    unsigned int id = glCreateShader(shader_type);

    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        exit(-1);
    }

    return id;
}
unsigned int complie_shader_program(const std::string &vs_path, const std::string &ps_path) {
    std::string vs_src = read_whole_file(vs_path);
    std::string ps_src = read_whole_file(ps_path);

    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, ps);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
        exit(-1);
    }

    glDeleteShader(vs);
    glDeleteShader(ps);

    return shaderProgram;
}
FIBITMAP *freeimage_load_and_convert_image(const std::string &image_path, bool is_color=true) {
    FIBITMAP *pImage_ori = FreeImage_Load(FreeImage_GetFileType(image_path.c_str(), 0), image_path.c_str());
    if (pImage_ori == nullptr) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        exit(-1);
    }
    FIBITMAP *pImage = FreeImage_ConvertTo24Bits(pImage_ori);
    FreeImage_FlipVertical(pImage); // 翻转，适应opengl的方向
    if (is_color) {
        FreeImage_AdjustGamma(pImage, 1 / 2.2);// 对于颜色贴图，进行矫正
    }
    FreeImage_Unload(pImage_ori);

    return pImage;
}

void RenderReousce::load_gltf(const std::string &base_key, const std::string &path) {
    std::string root = path;
    for (; !(root.empty() || root.back() == '/' || root.back() == '\\'); root.pop_back())
        ;

    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        reader.parse(file, json, false);
    }
    
    struct Buffer {
        char *ptr = nullptr;
        size_t len;
        Buffer(size_t len) : ptr(new char[len]), len(len) {}
        ~Buffer() {
            if (ptr != nullptr) {
                delete[] ptr;
            }
        }
    };
    std::vector<Buffer> buffers;
    if (json.isMember("buffers")) {
        for (const Json::Value &buffer : json["buffers"]) {
            std::string bin_path = root + buffer["uri"].asString();
            Buffer &b = buffers.emplace_back(buffer["byteLength"].asUInt());
            FILE *read;
            if (fopen_s(&read, bin_path.c_str(), "rb") != 0) {
                std::cerr << "Falied to read file: " << bin_path << std::endl;
                exit(-1);
            }
            fread((char *)b.ptr, 1, b.len, read);
            fclose(read);
        }
    }
    auto get_buffer = [&](uint32_t accessor_id) -> const Json::Value & {
        const Json::Value &buffer_view = json["bufferViews"][json["accessors"][accessor_id]["bufferView"].asUInt()];
        return buffer_view;
    };
    // 加载网格
    if (json.isMember("meshes")) {
        for (const Json::Value &mesh : json["meshes"]) {
            const std::string &key = base_key + '.' + mesh["name"].asString();
            const Json::Value &primitive = mesh["primitives"][0];
            const Json::Value &indices_buffer = get_buffer(primitive["indices"].asInt64());
            const Json::Value &position_buffer = get_buffer(primitive["attributes"]["POSITION"].asInt64());
            const Json::Value &normal_buffer = get_buffer(primitive["attributes"]["NORMAL"].asInt64());
            const Json::Value &uv_buffer = get_buffer(primitive["attributes"]["TEXCOORD_0"].asInt64());
            const Json::Value &tangent_buffer = get_buffer(primitive["attributes"]["TANGENT"].asInt64());

            uint32_t indices_count = json["accessors"][primitive["indices"].asUInt()]["count"].asInt64();
            uint16_t *indices_ptr = (uint16_t *)((char *)buffers[indices_buffer["buffer"].asInt64()].ptr +
                                                 indices_buffer["byteOffset"].asInt64());

            uint32_t vertex_count = position_buffer["byteLength"].asInt64() / sizeof(Vector3f);
            uint32_t normal_count = normal_buffer["byteLength"].asInt64() / sizeof(Vector3f);
            uint32_t uv_count = uv_buffer["byteLength"].asInt64() / sizeof(Vector2f);
            uint32_t tangent_count = tangent_buffer["byteLength"].asInt64() / sizeof(Vector4f);
            assert(vertex_count == normal_count && vertex_count == uv_count && vertex_count == tangent_count);
            Vector3f *pos = (Vector3f *)((char *)buffers[position_buffer["buffer"].asInt64()].ptr +
                                         position_buffer["byteOffset"].asInt64());
            Vector3f *normal = (Vector3f *)((char *)buffers[normal_buffer["buffer"].asInt64()].ptr +
                                            normal_buffer["byteOffset"].asInt64());
            Vector2f *uv =
                (Vector2f *)((char *)buffers[uv_buffer["buffer"].asInt64()].ptr + uv_buffer["byteOffset"].asInt64());
            Vector4f *tangent = (Vector4f *)((char *)buffers[tangent_buffer["buffer"].asInt64()].ptr +
                                             tangent_buffer["byteOffset"].asInt64());

            std::vector<Vertex> vertices(vertex_count);
            for (uint32_t i = 0; i < vertex_count; i++) {
                // tangent的第四个分量是用来根据平台决定手性的，在opengl中始终应该取1，所以忽略
                Vector3f tang = Vector3f(tangent[i].x, tangent[i].y, tangent[i].z);
                vertices[i] = {pos[i], normal[i], tang, uv[i]};
            }

            add_mesh(key, vertices.data(), vertices.size(), indices_ptr, indices_count);
        }
    }
    // 加载纹理（在加载材质时加载需要的纹理）
    auto load_texture = [&](uint32_t index, bool is_color) -> std::string {
        const Json::Value &texture = json["images"][index];
        const std::string key = base_key + '.' + texture["name"].asString();
        add_texture(key, root + texture["uri"].asString(), is_color);
        return key;
    };

    // 加载材质
    if (json.isMember("materials")) {
        for (const Json::Value &material : json["materials"]) {
            const std::string &key = base_key + '.' + material["name"].asString();

            std::string normal_texture = "default_normal";
            if (material.isMember("normalTexture")) {
                normal_texture = load_texture(material["normalTexture"]["index"].asInt64(), false);
            }
            const std::string basecolor_texture =
                load_texture(material["pbrMetallicRoughness"]["baseColorTexture"]["index"].asInt64(), true);
            std::string metallic_roughness_texture = "white";
            if (material["pbrMetallicRoughness"].isMember("metallicRoughnessTexture")) {
                metallic_roughness_texture =
                    load_texture(material["pbrMetallicRoughness"]["metallicRoughnessTexture"]["index"].asInt64(), false);
            }

            float metallicFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("metallicFactor")) {
                metallicFactor = material["pbrMetallicRoughness"]["metallicFactor"].asFloat();
            }
            float roughnessFactor = 1.0f;
            if (material["pbrMetallicRoughness"].isMember("roughnessFactor")) {
                roughnessFactor = material["pbrMetallicRoughness"]["roughnessFactor"].asFloat();
            }
            float uniform_data[2] = {metallicFactor, roughnessFactor};

            MaterialDesc desc = {"pbr",
                                 {{2, sizeof(float) * 2, &uniform_data}},
                                 {{0, basecolor_texture}, {1, normal_texture}, {2, metallic_roughness_texture}}};

            add_material(key, desc);
        }
    }
}
void RenderReousce::load_json(const std::string &path) {
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        reader.parse(file, json, false);
    }

    std::string base_dir;
    if (size_t it = path.find_last_of("/\\"); it != std::string::npos) {
        base_dir = path.substr(0, it + 1); // 包含'/'
    } else {
        base_dir = ""; // 可能在同一目录下
    }

    if (json.isMember("shader")) {
        for (const auto &key: json["shader"].getMemberNames()) {
            const Json::Value& shader_desc = json["shader"][key];
            add_shader(key, base_dir + shader_desc["vs_path"].asString(),
                       base_dir + shader_desc["ps_path"].asString());
        }
    }

    if (json.isMember("texture")) {
        for (const auto &key: json["texture"].getMemberNames()) {
            const Json::Value& texture_desc = json["texture"][key];
            bool is_color = texture_desc.isMember("is_color") ? texture_desc["is_color"].asBool() : true;
            add_texture(key, base_dir + texture_desc["image"].asString(), is_color);
        }
    }

    if (json.isMember("cubemap")) {
        for (const auto &key : json["cubemap"].getMemberNames()) {
            const Json::Value& cubemap_desc =  json["cubemap"][key];
            add_cubemap(key, base_dir + cubemap_desc["px"].asString(), base_dir + cubemap_desc["nx"].asString(),
                        base_dir + cubemap_desc["py"].asString(), base_dir + cubemap_desc["ny"].asString(),
                        base_dir + cubemap_desc["pz"].asString(), base_dir + cubemap_desc["nz"].asString());
        }
    }

    if (json.isMember("gltf")) {
        for (const auto &key : json["gltf"].getMemberNames()) {
            load_gltf(key, base_dir + json["gltf"][key]["path"].asString());
        }
    }
}
void RenderReousce::clear() {
    meshes.clear();
    materials.clear();
    shaders.clear();
    cubemaps.clear();
    textures.clear();
    for (auto &de : deconstructors) {
        de();
    }
    deconstructors.clear();
}
void RenderReousce::add_shader(const std::string &key, const std::string &vs_path, const std::string &ps_path) {
    std::cout << "Load shader: " << key << std::endl;
    unsigned int program_id = complie_shader_program(vs_path, ps_path);

    shaders.add(key, Shader{program_id});

    deconstructors.emplace_back([program_id]() { glDeleteProgram(program_id); });
}
void RenderReousce::add_cubemap(const std::string &key, const std::string &image_px, const std::string &image_nx,
                                const std::string &image_py, const std::string &image_ny, const std::string &image_pz,
                                const std::string &image_nz) {
    std::cout << "Load skybox: " << key << std::endl;

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
    std::cout << "Load texture: " << key << std::endl;

    FIBITMAP *pImage = freeimage_load_and_convert_image(image_path, is_color);

    unsigned int nWidth = FreeImage_GetWidth(pImage);
    unsigned int nHeight = FreeImage_GetHeight(pImage);

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_BGR, GL_UNSIGNED_BYTE,
                 (void *)FreeImage_GetBits(pImage));
    glGenerateTextureMipmap(texture_id);

    checkError();

    FreeImage_Unload(pImage);

    glBindTexture(GL_TEXTURE_2D, 0);

    textures.add(key, Texture{texture_id});

    deconstructors.emplace_back([texture_id]() { glDeleteTextures(1, &texture_id); });
}
void RenderReousce::add_material(const std::string &key, const MaterialDesc &desc) {
    std::cout << "Load material: " << key << std::endl;
    Material mat;
    mat.shaderprogram_id = shaders.get(shaders.find(desc.shader_name)).program_id;
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
        mat.samplers.emplace_back(
            Material::SampleData{s.binding_id, textures.get(textures.find(s.texture_key)).texture_id});
    }

    materials.add(key, std::move(mat));
}
void RenderReousce::add_mesh(const std::string &key, const Vertex *vertices, size_t vertex_count,
                             const uint16_t *const indices, size_t indices_count) {
    std::cout << "Load mesh: " << key << std::endl;
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
void Material::bind() const {
    glUseProgram(shaderprogram_id); // 绑定此材质关联的着色器

    // 绑定材质的uniform buffer
    for (const UniformData &u : uniforms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, u.binding_id, u.buffer_id);
    }
    // 绑定所有纹理
    for (const SampleData &s : samplers) {
        glActiveTexture(GL_TEXTURE0 + s.binding_id);
        glBindTexture(GL_TEXTURE_2D, s.texture_id);
    }
}
