#pragma once

#include "core/RefCount.h"
#include "core/hash_helper.h"
#include "core/log/Log.h"
#include "platform/graphics/MaterialParameter.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "runtime/GAssert.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Goonya {

enum class BufferBindingType { UNIFORM, SHADER_STORAGE };

struct MaterialParameterInfo {
    MaterialParameter type_and_default_value;
    size_t offset = 0;
};

struct MaterialParameterBlockInfo {
    std::unordered_map<std::string, MaterialParameterInfo> fields; // name -> (type, default value, offset)
    uint32_t total_size = 0;
};

struct TextureParameterInfo {
    TextureType type = TextureType::UNKNOWN;
    uint32_t unit = 0;
};

struct ImageParameterInfo {
    TextureType type = TextureType::UNKNOWN;
    uint32_t unit = 0;
    uint32_t gl_access = 0; // 着色器访问图像的模式
    uint32_t gl_type = 0; // 着色器中要求的图像类型
};

// 着色器中图像变量的声明信息，来自着色器文件的#image指令
struct ImageDecl {
    uint32_t gl_access = 0; // GL_READ_ONLY / GL_WRITE_ONLY / GL_READ_WRITE
    uint32_t gl_type = 0;   // 着色器要求的图像格式，如GL_RGBA32F
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
    std::array<uint32_t, 3> work_group_size{0, 0, 0}; // 计算着色器工作组的大小，光栅化着色器恒为0

public:
    ~GLShader() { glDeleteProgram(id); }
    static Ref<GLShader> compile_graphics(const std::string &vs_src, const std::string &ps_src);
    static Ref<GLShader> compile_compute(const std::string &src);

    bool is_graphics_shader() const noexcept { return category == ShaderCategory::GRAPHICS; }

    bool is_compute_shader() const noexcept { return category == ShaderCategory::COMPUTE; }
    const std::array<uint32_t, 3> &get_work_group_size() const noexcept { return work_group_size; }
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

    std::array<uint32_t, 3> get_compute_work_group_size() const noexcept;
    // 图像变量的gl_access/gl_type无法通过OpenGL反射获取，只能在着色器加载时通过#image指令声明，
    // 因此这里只返回名称和纹理类型，unit由UberShader手动分配
    std::unordered_map<std::string, TextureType> get_image_info() const noexcept;
};

} // namespace Goonya
