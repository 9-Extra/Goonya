#include "shaderlib.h"
#include "core/metatype/metatype.h"
#include "runtime/GoonyaException.h"

#include <format>
#include <iostream>
#include <sstream>

namespace Goonya {
namespace Graphics {

unsigned int complie_shader(const char *const src, unsigned int shader_type) {
    unsigned int id = glCreateShader(shader_type);

    glShaderSource(id, 1, &src, NULL);
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
GLuint complie_shader_program(const std::string &vs_src, const std::string &ps_src) {
    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, ps);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        throw RuntimeError(std::format("着色器链接错误： {}", infoLog));
    }

    glDeleteShader(vs);
    glDeleteShader(ps);

    return shaderProgram;
}

std::string mix_shader_definations(const std::string &src,
                                   const std::unordered_map<std::string, std::string> &definations) {
    std::stringstream ss;
    size_t first_line = src.find('\n');
    ss << src.substr(0, first_line) << std::endl;

    ss << "//------Combined Definations---------: \n";

    for (const auto &[k, v] : definations) {
        ss << std::format("#ifdef {0}\n#undef {0}\n#endif\n#define {0} {1}\n", k, v);
    }

    ss << "//------Combined Defination End------: \n";

    ss << src.substr(first_line);
    return ss.str();
}

ShaderResource ShaderLib::load_shader(const Resource::ShaderDesc &desc) {
    const UberShaderSource &u_src = uber_shader_sources.at(desc.get_uber_name());

    std::string mixed_vs = mix_shader_definations(u_src.vs_src, desc.get_definations());
    std::string mixed_ps = mix_shader_definations(u_src.ps_src, desc.get_definations());

    // std::ofstream(desc.uber_name + "_vs_mixed.vert", std::ios_base::binary) << mixed_vs;
    // std::ofstream(desc.uber_name + "_ps_mixed.frag", std::ios_base::binary) << mixed_ps;

    GLuint id = complie_shader_program(mixed_vs, mixed_ps);
    ShaderIntrospector introspector(id);
    auto buffer_info = introspector.get_constant_buffer_info();
    ShaderResource resource{
        id,
        std::move(buffer_info["per_object"]),
        std::move(buffer_info["per_material"]),
        std::move(buffer_info["per_frame"]),
    };

    return resource;
}

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

std::unordered_map<std::string, Meta::LayoutInfo> ShaderIntrospector::get_constant_buffer_info() const noexcept {
    // 获取所有uniform_block内部所有字段和偏移量
    GLint uniform_block_num;
    glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_num);
    const GLenum uniform_block_properties[] = {GL_NAME_LENGTH, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
    const size_t property_count = std::extent_v<decltype(uniform_block_properties)>;

    std::unordered_map<std::string, Meta::LayoutInfo> result;
    result.reserve(uniform_block_num);

    for (int i = 0; i < uniform_block_num; ++i) {
        GLint values[property_count];
        // 获取名称长度，总大小，和字段数量
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, property_count, uniform_block_properties, property_count, NULL,
                               values);
        checkError();
        auto [name_len, size, var_num] = values;
        // 获取块名称
        std::string name;
        // OpenGL返回的名称长度包括尾部的'\n'，但c++的不需要，同时后面的bufSize也是假定包括'\n'的。
        name.resize(name_len - 1);
        glGetProgramResourceName(id, GL_UNIFORM_BLOCK, i, name_len, NULL, name.data());
        // 获取内部所有字段id
        std::vector<GLint> unifrom_ids(var_num);
        const GLenum var_property[] = {GL_ACTIVE_VARIABLES};
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, 1, var_property, var_num, NULL, unifrom_ids.data());

        std::unordered_map<std::string, Meta::Field> fields;
        for (int i = 0; i < var_num; ++i) {
            const GLenum uniform_properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};
            const size_t property_count = std::extent_v<decltype(uniform_properties)>;
            // 获取内部字段名称长度，类型和偏移量
            GLint values[property_count];
            glGetProgramResourceiv(id, GL_UNIFORM, unifrom_ids[i], property_count, uniform_properties, property_count,
                                   NULL, values);
            auto [name_len, type, offset] = values;
            // 获取字段名称
            std::string field_name;
            field_name.resize(name_len - 1);
            glGetProgramResourceName(id, GL_UNIFORM, unifrom_ids[i], name_len, NULL, field_name.data());
            fields.emplace(field_name, Meta::Field{GLType2FieldType(type), (size_t)offset});
        }

        result.emplace(name, Meta::LayoutInfo{std::move(fields), (size_t)size});
    }

    checkError();

    return result;
}
} // namespace Graphics
} // namespace Goonya