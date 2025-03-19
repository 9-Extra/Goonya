#pragma once

#include "core/metatype/metatype.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBasic.h"
#include "platform/read_file.h"

namespace Goonya {
namespace Graphics {

struct ShaderUniformBlockInfo {
    Meta::LayoutInfo layout;
    GLuint binding;
};

class GLShader : public Shader {
public:
    GLShader(GLuint id);
    ~GLShader() { glDeleteProgram(program_id); }

    virtual void bind() override { glUseProgram(program_id); }

    const ShaderUniformBlockInfo &get_per_material_layout() const noexcept { return per_material; }
    const ShaderUniformBlockInfo &get_per_frame_layout() const noexcept { return per_frame; }
    const std::unordered_map<std::string, GLuint> get_texture_units() const noexcept { return texture_units; }

protected:
    GLuint program_id;
    ShaderUniformBlockInfo per_material;
    ShaderUniformBlockInfo per_frame;

    std::unordered_map<std::string, GLuint> texture_units;
};

// -------------------模拟PSO-----------------------

class GLPipelineStateObject : public PipelineStateObject {
public:
    GLPipelineStateObject(const PSODesc &desc);
    virtual void bind() const override;
    intrusive_ptr<GLShader> &get_shader() noexcept { return shader; }

private:
    friend class GLMaterial;
    friend class PSOCache;

    bool enable_cilp;            // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode;       // glCullFace
    GLenum front_face_clockwise; // glFrontFace

    bool enable_depth_test;
    GLenum depth_func;

    intrusive_ptr<GLShader> shader;
};

// -------------------ShaderLib------------------------

class GLShaderLib : public ShaderLib {
protected:
    intrusive_ptr<Shader> load_shader(const ShaderDesc &desc) override;
    virtual intrusive_ptr<PipelineStateObject> load_pso(const PSODesc &desc) override {
        return intrusive_ptr<PipelineStateObject>{new GLPipelineStateObject(desc)};
    }
};

} // namespace Graphics
} // namespace Goonya