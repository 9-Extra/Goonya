#pragma once

#include "../GraphicsResource.h"
#include "core/intrusive_ptr.h"
#include "GLBuffer.h"
#include "platform/graphics/opengl/shaderlib/shaderlib.h"

#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLShader: public Shader {
public:
    virtual void bind() override{
        glUseProgram(program_id);     
    } 
private:
    GLShader(GLuint id): program_id(id) {}
    GLuint program_id;
};

class GLPipelineStateObject: public PipelineStateObject{
public:
    GLPipelineStateObject(const Resource::PSODesc &desc);
    ~GLPipelineStateObject() override{
        glDeleteVertexArrays(1, &vao_id);
    };
    virtual void bind() const override;

private:
    friend class PSOCache;

    bool enable_cilp;            // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode;       // glCullFace
    GLenum front_face_clockwise; // glFrontFace

    bool enable_depth_test;
    GLenum depth_func;

    ShaderResource shader;
    GLuint vao_id; // 使用vao记录顶点内存布局，但是不绑定buffer
};


class GLMaterial: public Material {
    friend class OpenGLGraphicsAPI;
public:
    virtual void bind() const override;
    ~GLMaterial(){
        for(const auto& u: uniforms){
            glDeleteBuffers(1, &u.buffer_id);
        }
    }

private:
    GLMaterial() {}
    
    struct UniformData {
        GLuint binding_id;
        GLuint buffer_id;
    };
    struct SampleData {
        GLuint binding_id;
        intrusive_ptr<Texture> texture;
    };
    intrusive_ptr<GLPipelineStateObject> pipeline_state;
    std::vector<UniformData> uniforms;
    std::vector<SampleData> samplers;
};

} // namespace Graphics

} // namespace Goonya