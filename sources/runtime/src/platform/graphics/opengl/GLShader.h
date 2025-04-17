#pragma once

#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/read_file.h"
#include <cassert>

namespace Goonya::Graphics {

class GLShader final : public Shader {
public:
    GLShader(const std::string &vs_src, const std::string &ps_src);
    ~GLShader() override { glDeleteProgram(id); }

    void bind() override { glUseProgram(id); }
    GLuint get_id() const { return id; }

protected:
    GLuint id;
};

class GLShaderIntrospector final : public ShaderIntrospector {
public:
    explicit GLShaderIntrospector(Shader *shader) {
        GLShader *s = dynamic_cast<GLShader *>(shader);
        assert(s);
        id = s->get_id();
    }

    std::unordered_map<std::string, ShaderUniformBlockInfo> get_constant_buffer_info() const noexcept override;
    std::unordered_map<std::string, uint32_t> get_texture_info() const noexcept override;

private:
    GLuint id;
};

} // namespace Goonya::Graphics
