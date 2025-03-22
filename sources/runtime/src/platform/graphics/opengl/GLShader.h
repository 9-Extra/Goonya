#pragma once

#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/read_file.h"

namespace Goonya {
namespace Graphics {

class GLShader : public Shader {
public:
    GLShader(GLuint id, const UberShader* uber_shader, VariantCodeSet code): Shader(uber_shader, code), id(id) {};
    ~GLShader() { glDeleteProgram(id); }

    virtual void bind() override { glUseProgram(id); }

protected:
    GLuint id;
};

// -------------------ShaderLib------------------------

class GLShaderLib : public ShaderLib {
protected:
    virtual void add_uber_shader(const std::string &name, UberShaderDesc &&desc) override;
    virtual intrusive_ptr<Shader> create_variant(UberShader *uber_shader, VariantCodeSet variant_code) override;

};

} // namespace Graphics
} // namespace Goonya