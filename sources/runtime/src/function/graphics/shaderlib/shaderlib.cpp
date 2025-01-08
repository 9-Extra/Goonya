#include "shaderlib.h"

#include <iostream>
#include <sstream>
#include <format>

namespace Goonya{
namespace Graphics{

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
GLuint complie_shader_program(const std::string &vs_src, const std::string &ps_src) {
    unsigned int vs = complie_shader(vs_src.c_str(), GL_VERTEX_SHADER);
    unsigned int ps = complie_shader(ps_src.c_str(), GL_FRAGMENT_SHADER);

    GLuint shaderProgram;
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

std::string mix_shader_definations(const std::string& src, const std::unordered_map<std::string, std::string>& definations){
    std::stringstream ss;
    size_t first_line = src.find('\n');
    ss << src.substr(0, first_line) << std::endl;

    ss << "//------Combined Definations---------: \n";

    for(const auto& [k, v] : definations){
        ss << std::format("#define {} {}\n", k, v);
    }

    ss << "//------Combined Defination End------: \n";

    ss << src.substr(first_line);
    return ss.str();
}

ShaderResource ShaderLib::load_shader(const Resource::ShaderDesc& desc){
    const UberShaderSource& u_src = uber_shader_sources.at(desc.get_uber_name());

    std::string mixed_vs = mix_shader_definations(u_src.vs_src, desc.get_definations());
    std::string mixed_ps = mix_shader_definations(u_src.ps_src, desc.get_definations());

    //std::ofstream(desc.uber_name + "_vs_mixed.vert", std::ios_base::binary) << mixed_vs;
    //std::ofstream(desc.uber_name + "_ps_mixed.frag", std::ios_base::binary) << mixed_ps;

    GLuint id = complie_shader_program(mixed_vs, mixed_ps);
    ShaderResource resource{id};
    
    return resource;
}

std::vector<std::tuple<std::string, ConstantBufferInfo>> ShaderIntrospector::get_constant_buffer_info() const noexcept {
    GLint uniform_block_num;
    glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_num);
    const GLenum uniform_block_properties[] = {GL_NAME_LENGTH, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
    const size_t property_count = std::extent_v<decltype(uniform_block_properties)>;

    std::vector<std::tuple<std::string, ConstantBufferInfo>> result;
    result.reserve(uniform_block_num);

    for (int i = 0; i < uniform_block_num; ++i) {
        GLint values[property_count];
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, property_count, uniform_block_properties,
                               property_count, NULL, values);
        checkError();
        auto [name_len, size, var_num] = values;

        std::string name;
        name.resize(name_len);
        glGetProgramResourceName(id, GL_UNIFORM_BLOCK, i, name.size(), NULL, name.data());

        std::vector<GLint> unifrom_ids(var_num);
        const GLenum var_property[] = {GL_ACTIVE_VARIABLES};
        glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, 1, var_property, var_num, NULL, unifrom_ids.data());

        std::unordered_map<std::string, ConstantBufferInfo::FieldInfo> fields;
        for (int i = 0; i < var_num; ++i) {
            const GLenum uniform_properties[] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};
            const size_t property_count = std::extent_v<decltype(uniform_properties)>;
            GLint values[property_count];
            glGetProgramResourceiv(id, GL_UNIFORM, unifrom_ids[i], property_count, uniform_properties,
                                   property_count, NULL, values);
            auto [name_len, type, offset] = values;

            std::string name;
            name.resize(name_len + 1);
            glGetProgramResourceName(id, GL_UNIFORM, unifrom_ids[i], name.size(), NULL, name.data());
            fields.emplace(std::move(name), ConstantBufferInfo::FieldInfo{(Meta::FieldType)type, (size_t)offset});
        }

        ConstantBufferInfo info{.name = std::move(name), .total_size = (size_t)size, .fields = std::move(fields)};

        result.emplace_back(std::move(name), std::move(info));
    }

    checkError();

    return result;
}
} // namespace Graphics
}