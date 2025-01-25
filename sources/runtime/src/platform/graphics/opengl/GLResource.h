#pragma once

#include "../GraphicsResource.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/graphics.h"
#include "GLBuffer.h"
#include "platform/graphics/opengl/shaderlib/shaderlib.h"

#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLMesh : public Mesh {
public:
    virtual void bind() override{
        glBindVertexArray(vao_id);     
    }   

    virtual uint32_t get_indices_count() override{
        return index_buffer->get_index_count();
    }

    ~GLMesh() {
        glDeleteVertexArrays(1, &vao_id);
        checkError();
    }
private:
    GLuint vao_id;
    GLenum topology;
    intrusive_ptr<GLVertexBuffer> vertex_buffers;
    intrusive_ptr<GLIndexBuffer> index_buffer;

    friend class OpenGLGraphicsAPI;
    GLMesh(GLuint vao_id, GLenum topology, intrusive_ptr<GLVertexBuffer> vertex_buffers, intrusive_ptr<GLIndexBuffer> index_buffer)
    : vao_id(vao_id), topology(topology), vertex_buffers(vertex_buffers), index_buffer(index_buffer)
    {}
};

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
    ~GLPipelineStateObject() override{};
    virtual void bind() const override;

private:
    friend class PSOCache;
    GLPipelineStateObject() {}
    
    bool enable_cilp;            // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode;       // glCullFace
    GLenum front_face_clockwise; // glFrontFace

    bool enable_depth_test;
    GLenum depth_func;

    ShaderResource shader;
    
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
        GLenum texture_type;
        GLenum min_filter;
        GLenum mag_filter;
        GLenum warp_mode;
    };
    intrusive_ptr<GLPipelineStateObject> pipeline_state;
    std::vector<UniformData> uniforms;
    std::vector<SampleData> samplers;
};

} // namespace Graphics

} // namespace Goonya