#pragma once

#include "core/RefCount.h"
#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLTexture.h"


#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Goonya {

enum class BufferBindingType {
    UNIFORM,
    SHADER_STORAGE
};

struct MaterialParameterInfo{
    MaterialParameter type_and_default_value;
    size_t offset;
};

struct MaterialParameterBlockInfo {
    std::unordered_map<std::string, MaterialParameterInfo> fields; // name -> (type, default value, offset)
    uint32_t total_size = 0;
    uint32_t binding = 0;
};

struct TextureParameterInfo{
    TextureType type = TextureType::UNKNOWN;
    uint32_t unit = 0;
};


class GLShader final : public RefCount {
private:
    GLuint id = 0;

public:
    GLShader(const std::string &vs_src, const std::string &ps_src);
    ~GLShader() { glDeleteProgram(id); }

    void bind() const noexcept { glUseProgram(id); }

    void set_texture_binding(const std::string &name, uint32_t unit) const noexcept{
        GLint location = glGetUniformLocation(id, name.c_str());
        if (location != -1){
            glProgramUniform1i(id, location, unit);
        } else {
            LOG_WARN("着色器中未找到纹理{}", name);
        }
    }
    void set_texture_binding(uint32_t location, uint32_t unit) const noexcept{
        glProgramUniform1i(id, location, unit);
    }
    GLuint get_id() const { return id; }
};

class GLShaderIntrospector final {
private:
    GLuint id;

public:
    explicit GLShaderIntrospector(GLShader *shader) {
        GN_ASSERT(shader);
        id = shader->get_id();
    }

    MaterialParameterBlockInfo get_per_material_uniform_info() const noexcept;
    std::unordered_map<std::string, std::tuple<uint32_t, BufferBindingType>, StringHash, StringEqual> get_uniform_binding_info() const noexcept;
    std::unordered_map<std::string, TextureType>  get_texture_info() const noexcept;
};

} // namespace Goonya
