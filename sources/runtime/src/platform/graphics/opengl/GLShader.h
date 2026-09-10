#pragma once

#include "core/RefCount.h"
#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GAssert.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Goonya {

enum class BufferBindingType { UNIFORM, SHADER_STORAGE };

struct MaterialParameterInfo {
    MaterialParameter type_and_default_value;
    size_t offset;
};

struct MaterialParameterBlockInfo {
    std::unordered_map<std::string, MaterialParameterInfo> fields; // name -> (type, default value, offset)
    uint32_t total_size = 0;
};

struct TextureParameterInfo {
    TextureType type = TextureType::UNKNOWN;
    uint32_t unit = 0;
};

// OpenGL允许一个着色器既包含光栅化管线的着色器又包含计算着色器，但是Goonya要求两者分开到不同着色器对象中
enum class ShaderCategory { NONE, GRAPHICS, COMPUTE };

/**
 * @brief 编译完成的光栅化着色器组合，即shaderprogram
 */
class GLShader final : public RefCount {
private:
    GLuint id = 0;
    ShaderCategory category = ShaderCategory::NONE;

public:
    ~GLShader() { glDeleteProgram(id); }
    static Ref<GLShader> compile_graphics(const std::string &vs_src, const std::string &ps_src);
    static Ref<GLShader> compile_compute(const std::string &src);

    bool is_graphics_shader() const noexcept { return category == ShaderCategory::GRAPHICS; }

    bool is_compute_shader() const noexcept { return category == ShaderCategory::COMPUTE; }
    void bind_draw() const noexcept {
        GN_ASSERT(is_graphics_shader());
        glUseProgram(id);
    }

    void dispatch_compute(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) const {
        GN_ASSERT(is_compute_shader());
        glUseProgram(id);
        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    }

    void set_texture_binding(const std::string &name, uint32_t unit) const noexcept {
        GLint location = glGetUniformLocation(id, name.c_str());
        if (location != -1) {
            glProgramUniform1i(id, location, unit);
        } else {
            LOG_WARN("着色器中未找到纹理{}", name);
        }
    }
    void set_texture_binding(uint32_t location, uint32_t unit) const noexcept {
        glProgramUniform1i(id, location, unit);
    }
    GLuint get_id() const { return id; }

private:
    GLShader() { this->id = glCreateProgram(); }
};

class GLShaderIntrospector final {
private:
    GLuint id;

public:
    explicit GLShaderIntrospector(GLShader *shader) {
        GN_ASSERT(shader);
        id = shader->get_id();
    }

    MaterialParameterBlockInfo get_per_material_uniform_info() const;
    std::unordered_map<std::string, std::tuple<uint32_t, BufferBindingType>, StringHash, StringEqual>
    get_uniform_binding_info() const;
    std::unordered_map<std::string, TextureType> get_texture_info() const noexcept;
};

} // namespace Goonya
