#pragma once

#include "core/RefCount.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/opengl/GLBasic.h"
#include <cassert>
#include <string>
#include <unordered_map>

namespace Goonya {

enum class BufferBindingType {
    UNIFORM,
    SHADER_STORAGE
};

struct ShaderUniformBlockInfo final {
    Meta::LayoutInfo layout;
    uint32_t binding = 0;
    BufferBindingType binding_type = BufferBindingType::UNIFORM;
};


class GLShader final : public RefCount {
private:
    GLuint id = 0;

public:
    GLShader(const std::string &vs_src, const std::string &ps_src);
    ~GLShader() { glDeleteProgram(id); }

    void bind() const noexcept { glUseProgram(id); }
    GLuint get_id() const { return id; }
};

class GLShaderIntrospector final {
private:
    GLuint id;

public:
    explicit GLShaderIntrospector(GLShader *shader) {
        assert(shader);
        id = shader->get_id();
    }

    std::unordered_map<std::string, ShaderUniformBlockInfo> get_constant_buffer_info() const noexcept;
    std::unordered_map<std::string, uint32_t> get_texture_info() const noexcept;
};

} // namespace Goonya
