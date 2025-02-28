#pragma once

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <glad/glad.h>
#include <memory>

#include "../Material.h"
#include "GLBuffer.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
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
    ~GLPipelineStateObject() override { glDeleteVertexArrays(1, &vao_id); };
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
    GLuint vao_id; // 使用vao记录顶点内存布局，但是不绑定buffer
};

class GLMaterial : public Material {
public:
    GLMaterial(const intrusive_ptr<PipelineStateObject> &pso) : Material(pso) {
        const auto &layout = get_shader().per_material.layout;
        per_material = intrusive_ptr<GLUniformBuffer>(layout.size, BufferType::STATIC);
        for (const auto &[name, f] : layout.fields) {
            parameters[name] = Meta::DynamicData(); // 所有需要的值建立key
        }
        texture_info = get_shader().texture_units;
    }

    virtual void bind() const override;
    virtual void update_parameters() const noexcept override;

private:
    const ShaderResource &get_shader() const {
        auto pso = dynamic_cast<GLPipelineStateObject *>(this->pso.get());
        assert(pso);
        return pso->shader;
    }
};

} // namespace Graphics

} // namespace Goonya