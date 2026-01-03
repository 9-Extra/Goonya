#include "GLShader.h"
#include "core/log/Log.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GoonyaException.h"
#include <cassert>
#include <cstdint>
#include <tuple>
#include <vector>

namespace Goonya {

// -------------------------编译-------------------------------

unsigned int compile_shader(const std::string &source, unsigned int shader_type) {
    unsigned int id = glCreateShader(shader_type);

    const GLchar *data = source.c_str();
    GLint length = static_cast<GLint>(source.length());
    glShaderSource(id, 1, &data, &length);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        throw RuntimeError(std::format("着色器编译错误： {}", infoLog));
    }

    return id;
}

GLShader::GLShader(const std::string &vs_src, const std::string &ps_src) {
    unsigned int vs = compile_shader(vs_src, GL_VERTEX_SHADER);
    unsigned int ps = compile_shader(ps_src, GL_FRAGMENT_SHADER);

    this->id = glCreateProgram();

    glAttachShader(id, vs);
    glAttachShader(id, ps);
    glLinkProgram(id);

    int success;
    char infoLog[512];

    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, nullptr, infoLog);
        throw RuntimeError(std::format("着色器链接错误： {}", infoLog));
    }

    glDeleteShader(vs);
    glDeleteShader(ps);
};

// ------------------------反射-------------------------------

static MaterialParameter GLType2FieldType(GLint gl_type) {
    switch (gl_type) {
    case GL_UNSIGNED_INT:
        return 0;
    case GL_FLOAT:
        return 0.0f;
    case GL_FLOAT_VEC2:
        return Vector2f();
    case GL_FLOAT_VEC3:
        return Vector3f();
    case GL_FLOAT_VEC4:
        return Vector4f();
    case GL_FLOAT_MAT4:
        return Matrix4f();
    default:
        throw RuntimeError(std::format("不支持的GLSL类型： {}", gl_type));
    }
}

std::unordered_map<std::string, std::tuple<uint32_t, BufferBindingType>, StringHash, StringEqual>
GLShaderIntrospector::get_uniform_binding_info() const noexcept {
    // 获取所有uniform_block内部所有字段和偏移量
    GLint uniform_block_num, shader_storage_num;
    glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_num);
    glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &shader_storage_num);

    const GLenum QUREY_PROPERTIES[] = {GL_BUFFER_BINDING, GL_NAME_LENGTH, GL_BUFFER_DATA_SIZE};
    const size_t property_count = std::extent_v<decltype(QUREY_PROPERTIES)>;

    std::unordered_map<std::string, std::tuple<uint32_t, BufferBindingType>, StringHash, StringEqual> result;
    result.reserve(uniform_block_num + shader_storage_num);

    for (int index = 0; index < uniform_block_num + shader_storage_num; ++index) {
        GLenum interface = index < uniform_block_num ? GL_UNIFORM_BLOCK : GL_SHADER_STORAGE_BLOCK;
        BufferBindingType binding_type =
            index < uniform_block_num ? BufferBindingType::UNIFORM : BufferBindingType::SHADER_STORAGE;

        int i = index < uniform_block_num ? index : index - uniform_block_num;

        GLint values[property_count];
        // 获取名称长度，总大小，和字段数量
        glGetProgramResourceiv(id, interface, i, property_count, QUREY_PROPERTIES, property_count, nullptr, values);

        auto [binding, name_len, size] = values;
        // 获取块名称
        std::string name;
        // OpenGL返回的名称长度包括尾部的'\n'，但c++的不需要，同时后面的bufSize也是假定包括'\n'的。
        name.resize(name_len - 1);
        glGetProgramResourceName(id, interface, i, name_len, nullptr, name.data());

        result.emplace(name, std::make_tuple(static_cast<uint32_t>(binding), binding_type));
    }

    return result;
}

MaterialParameterBlockInfo GLShaderIntrospector::get_per_material_uniform_info() const noexcept {

    MaterialParameterBlockInfo block_info{
        .fields = {},
        .total_size = 0,
        .binding = 0,
    }; // 如果没有per_material块，返回空的block_info

    // 获取所有uniform_block内部所有字段和偏移量
    GLint uniform_block_num, shader_storage_num;
    glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_num);
    glGetProgramInterfaceiv(id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &shader_storage_num);

    const GLenum QUREY_PROPERTIES[] = {GL_BUFFER_BINDING, GL_NAME_LENGTH, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
    const size_t property_count = std::extent_v<decltype(QUREY_PROPERTIES)>;

    for (int index = 0; index < uniform_block_num + shader_storage_num; ++index) {
        GLenum interface = index < uniform_block_num ? GL_UNIFORM_BLOCK : GL_SHADER_STORAGE_BLOCK;
        BufferBindingType binding_type =
            index < uniform_block_num ? BufferBindingType::UNIFORM : BufferBindingType::SHADER_STORAGE;

        int i = index < uniform_block_num ? index : index - uniform_block_num;

        GLint values[property_count];
        // 获取名称长度，总大小，和字段数量
        glGetProgramResourceiv(id, interface, i, property_count, QUREY_PROPERTIES, property_count, nullptr, values);

        auto [binding, name_len, size, var_num] = values;
        std::string name;
        // OpenGL返回的名称长度包括尾部的'\n'，但c++的不需要，同时后面的bufSize也是假定包括'\n'的。
        name.resize(name_len - 1);
        glGetProgramResourceName(id, interface, i, name_len, nullptr, name.data());

        if (name != "per_material") {
            continue;
        }

        // 开始获取per_material块的内部信息

        block_info.binding = static_cast<uint32_t>(binding);
        block_info.total_size = static_cast<uint32_t>(size);
        assert(binding_type == BufferBindingType::UNIFORM);
        // 获取块名称

        // 获取内部所有字段id
        std::vector<GLint> uniform_ids(var_num);
        const GLenum var_property[] = {GL_ACTIVE_VARIABLES};
        glGetProgramResourceiv(id, interface, i, 1, var_property, var_num, nullptr, uniform_ids.data());

        for (int i = 0; i < var_num; ++i) {
            const GLenum UNIFORM_PROPERTIES[] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};
            const size_t property_count = std::extent_v<decltype(UNIFORM_PROPERTIES)>;
            // 获取内部字段名称长度，类型和偏移量
            GLint values[property_count];
            glGetProgramResourceiv(id, GL_UNIFORM, uniform_ids[i], property_count, UNIFORM_PROPERTIES, property_count,
                                   nullptr, values);
            auto [name_len, type, offset] = values;
            // 获取字段名称
            std::string field_name;
            field_name.resize(name_len - 1);
            glGetProgramResourceName(id, GL_UNIFORM, uniform_ids[i], name_len, nullptr, field_name.data());
            block_info.fields.emplace(field_name,
                                      MaterialParameterInfo{GLType2FieldType(type), static_cast<uint32_t>(offset)});
        }
    }

    return block_info;
}

std::unordered_map<std::string, TextureType> GLShaderIntrospector::get_texture_info() const noexcept {
    std::unordered_map<std::string, TextureType> result;

    // 获取活跃uniform变量数量
    GLint num_uniforms;
    glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

    const GLenum UNIFORM_PROPERTIES[] = {GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH};
    const size_t property_count = std::extent_v<decltype(UNIFORM_PROPERTIES)>;

    for (int i = 0; i < num_uniforms; ++i) {
        GLint properties[property_count];
        glGetProgramResourceiv(id, GL_UNIFORM, i, property_count, UNIFORM_PROPERTIES, property_count, nullptr,
                               properties);
        auto [block_index, uniform_type, name_len] = properties;
        if (block_index != -1) continue;

        // 判断是否是采样器类型
        TextureType texture_type = TextureType::UNKNOWN;
        bool is_sampler = false;
        switch (uniform_type) {
        case GL_SAMPLER_1D:
        case GL_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_SAMPLER_1D_SHADOW:
            texture_type = TextureType::TEXTURE_1D;
            is_sampler = true;
            break;
        case GL_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_SAMPLER_2D_SHADOW:
            texture_type = TextureType::TEXTURE_2D;
            is_sampler = true;
            break;
        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
            texture_type = TextureType::TEXTURE_3D;
            is_sampler = true;
            break;
        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
            texture_type = TextureType::TEXTURE_CUBEMAP;
            is_sampler = true;
            break;
        case GL_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
            texture_type = TextureType::TEXTURE_1D_ARRAY;
            is_sampler = true;
            break;
        case GL_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            texture_type = TextureType::TEXTURE_2D_ARRAY;
            is_sampler = true;
            break;
        case GL_SAMPLER_CUBE_MAP_ARRAY:
        case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
        case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
            texture_type = TextureType::TEXTURE_CUBEMAP_ARRAY;
            is_sampler = true;
            break;
        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            // 其他暂不支持的采样器类型（Buffer Texture和Multisample Texture）
            texture_type = TextureType::UNKNOWN;
            is_sampler = true;
        default:
            break;
        }

        if (!is_sampler) continue;

        if (texture_type == TextureType::UNKNOWN) {
            LOG_ERROR("使用了暂不支持的采样器类型");
            continue;
        }

        // 获取名称
        std::string name_buffer;
        name_buffer.resize(name_len - 1);
        glGetProgramResourceName(id, GL_UNIFORM, i, name_len, nullptr, name_buffer.data());
        result.emplace(std::move(name_buffer), texture_type);
    }

    return result;
}

} // namespace Goonya
