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

ShaderResource ShaderLib::load_shader(const ShaderDesc& desc){
    const UberShaderSource& u_src = uber_shader_sources.at(desc.get_uber_name());

    std::string mixed_vs = mix_shader_definations(u_src.vs_src, desc.get_definations());
    std::string mixed_ps = mix_shader_definations(u_src.ps_src, desc.get_definations());

    //std::ofstream(desc.uber_name + "_vs_mixed.vert", std::ios_base::binary) << mixed_vs;
    //std::ofstream(desc.uber_name + "_ps_mixed.frag", std::ios_base::binary) << mixed_ps;

    GLuint id = complie_shader_program(mixed_vs, mixed_ps);
    ShaderResource resource{id};
    
    return resource;
}

}
}