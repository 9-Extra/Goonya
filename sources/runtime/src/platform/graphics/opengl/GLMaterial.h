#pragma once

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <glad/glad.h>

#include "../Material.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/opengl/shaderlib/shaderlib.h"

namespace Goonya {
namespace Graphics {

class GLShader : public Shader {
public:
    virtual void bind() override { glUseProgram(program_id); }

private:
    GLShader(GLuint id) : program_id(id) {}
    GLuint program_id;
};

class GLPipelineStateObject : public PipelineStateObject {
public:
    GLPipelineStateObject(const Resource::PSODesc &desc);
    virtual void bind() const override;

private:
    friend class GLMaterial;
    friend class PSOCache;

    bool enable_cilp;            // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode;       // glCullFace
    GLenum front_face_clockwise; // glFrontFace

    bool enable_depth_test;
    GLenum depth_func;

    ShaderResource shader;
};

class GLMaterial : public Material {
public:
    GLMaterial(const Resource::PSODesc &pso) : Material(pso) {}

    virtual void bind() override;
    virtual void update() override;

private:
    void reset_pso();

    void update_parameter();

    const ShaderResource &get_shader() const {
        auto pso = dynamic_cast<GLPipelineStateObject *>(this->pso.get());
        assert(pso);
        return pso->shader;
    }
};

} // namespace Graphics

} // namespace Goonya