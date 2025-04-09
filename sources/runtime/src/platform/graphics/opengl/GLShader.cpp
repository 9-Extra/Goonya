#include "GLShader.h"
#include "platform/graphics/Shader.h"
#include <vector>

namespace Goonya {
namespace Graphics {

// -------------------------编译-------------------------------

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

GLShader::GLShader(const std::string &vs_src, const std::string &ps_src) {
    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    this->id = glCreateProgram();

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
};

// ------------------------反射-------------------------------

static Meta::FieldType GLType2FieldType(GLint gl_type) noexcept {
    switch (gl_type) {
    case GL_UNSIGNED_INT:
        return Meta::FieldType::u32;
    case GL_FLOAT:
        return Meta::FieldType::f32;
    case GL_FLOAT_VEC2:
        return Meta::FieldType::vec2f;
    case GL_FLOAT_VEC3:
        return Meta::FieldType::vec3f;
    case GL_FLOAT_VEC4:
        return Meta::FieldType::vec4f;
    case GL_FLOAT_MAT4:
        return Meta::FieldType::mat4f;
    case GL_DOUBLE:
        return Meta::FieldType::f64;
    }
    return Meta::FieldType::nul;
}

std::unordered_map<std::string, ShaderUniformBlockInfo>
GLShaderIntrospector::get_constant_buffer_info() const noexcept {
    // 获取所有uniform_block内部所有字段和偏移量
    GLint uniform_block_num;
    glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_num);
    const GLenum UNIFORM_BLOCK_PROPERTIES[] = {GL_BUFFER_BINDING, GL_NAME_LENGTH, GL_BUFFER_DATA_SIZE,
                                               GL_NUM_ACTIVE_VARIABLES};
    const size_t property_count = std::extent_v<decltype(UNIFORM_BLOCK_PROPERTIES)>;

    std::unordered_map<std::string, ShaderUniformBlockInfo> result;
    result.reserve(uniform_block_num);

    for (int i = 0; i < uniform_block_num; ++i) {
        GLint values[property_count];
        // 获取名称长度，总大小，和字段数量
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, property_count, UNIFORM_BLOCK_PROPERTIES, property_count, NULL,
                               values);

        auto [binding, name_len, size, var_num] = values;
        // 获取块名称
        std::string name;
        // OpenGL返回的名称长度包括尾部的'\n'，但c++的不需要，同时后面的bufSize也是假定包括'\n'的。
        name.resize(name_len - 1);
        glGetProgramResourceName(id, GL_UNIFORM_BLOCK, i, name_len, NULL, name.data());
        // 获取内部所有字段id
        std::vector<GLint> unifrom_ids(var_num);
        const GLenum var_property[] = {GL_ACTIVE_VARIABLES};
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, 1, var_property, var_num, NULL, unifrom_ids.data());

        std::unordered_map<std::string, Meta::FieldInfo> fields;
        for (int i = 0; i < var_num; ++i) {
            const GLenum UNIFORM_PROPERTIES[] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};
            const size_t property_count = std::extent_v<decltype(UNIFORM_PROPERTIES)>;
            // 获取内部字段名称长度，类型和偏移量
            GLint values[property_count];
            glGetProgramResourceiv(id, GL_UNIFORM, unifrom_ids[i], property_count, UNIFORM_PROPERTIES, property_count,
                                   NULL, values);
            auto [name_len, type, offset] = values;
            // 获取字段名称
            std::string field_name;
            field_name.resize(name_len - 1);
            glGetProgramResourceName(id, GL_UNIFORM, unifrom_ids[i], name_len, NULL, field_name.data());
            fields.emplace(field_name, Meta::FieldInfo{GLType2FieldType(type), (size_t)offset});
        }

        result.emplace(name,
                       ShaderUniformBlockInfo{Meta::LayoutInfo{std::move(fields), (size_t)size}, (GLuint)binding});
    }

    return result;
}

std::unordered_map<std::string, GLuint> GLShaderIntrospector::get_texture_info() const noexcept {
    std::unordered_map<std::string, GLuint> result;

    // 获取活跃uniform变量数量
    GLint num_uniforms;
    glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

    const GLenum UNIFORM_PROPERTIES[] = {GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH, GL_LOCATION};
    const size_t property_count = std::extent_v<decltype(UNIFORM_PROPERTIES)>;

    for (int i = 0; i < num_uniforms; ++i) {
        GLint properties[property_count];
        glGetProgramResourceiv(id, GL_UNIFORM, i, property_count, UNIFORM_PROPERTIES, property_count, nullptr,
                               properties);
        auto [block_index, uniform_type, name_len, location] = properties;
        if (block_index != -1)
            continue;

        // 判断是否是采样器类型
        bool is_sampler = false;
        switch (uniform_type) {
        // 标准采样器类型
        case GL_SAMPLER_1D:
        case GL_SAMPLER_2D:
        case GL_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_CUBE_MAP_ARRAY:
        case GL_SAMPLER_BUFFER:
        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        // 整数采样器类型
        case GL_INT_SAMPLER_1D:
        case GL_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_3D:
        case GL_INT_SAMPLER_CUBE:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        // 无符号整数采样器类型
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            is_sampler = true;
            break;
        default:
            break;
        }

        if (!is_sampler)
            continue;

        // 获取名称
        std::string name_buffer;
        name_buffer.resize(name_len - 1);
        glGetProgramResourceName(id, GL_UNIFORM, i, name_len, nullptr, name_buffer.data());

        GLuint texture_unit;
        glGetUniformuiv(id, location, &texture_unit); // 绑定的纹理单元视为对应location中存储的值
        result.emplace(name_buffer, texture_unit);
    }

    return result;
}

} // namespace Graphics
} // namespace Goonya