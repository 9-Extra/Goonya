#pragma once

#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/read_file.h"
#include <cassert>

namespace Goonya {
namespace Graphics {

class GLShader : public Shader {
public:
    GLShader(const std::string &vs_src, const std::string &ps_src);
    ~GLShader() { glDeleteProgram(id); }

    virtual void bind() override { glUseProgram(id); }
    GLuint get_id() const {
        return id;
    }

protected:
    GLuint id;
};

class GLShaderIntrospector: public ShaderIntrospector {
public:
    GLShaderIntrospector(Shader* shader) {
        GLShader* s = dynamic_cast<GLShader*>(shader);
        assert(s);
        id = s->get_id();
    }

    virtual std::unordered_map<std::string, ShaderUniformBlockInfo> get_constant_buffer_info() const noexcept;
    virtual std::unordered_map<std::string, uint32_t> get_texture_info() const noexcept;
private:
    GLuint id;
};

} // namespace Graphics
} // namespace Goonya