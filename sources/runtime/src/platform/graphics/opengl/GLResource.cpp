#include "GLResource.h"

#include "GLTexture.h"
#include "platform/graphics/opengl/GLBuffer.h"

namespace Goonya {
namespace Graphics {

void GLPipelineStateObject::bind() const {
    if (this->enable_cilp) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    glCullFace(this->cull_face_mode);
    glFrontFace(this->front_face_clockwise);
    if (this->enable_depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthFunc(this->depth_func);
    glUseProgram(this->shader.gl_id); // 绑定着色器
};

void GLMaterial::bind() const {
    checkError();
    pipeline_state->bind(); // 绑定此材质关联的着色器
    // 绑定材质的uniform buffer
    for (const GLMaterial::UniformData &u : uniforms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, u.binding_id, u.buffer_id);
    }
    // 绑定所有纹理
    for (const GLMaterial::SampleData &s : samplers) {
        GLuint texture_id = dynamic_cast<GLTextureBase*>(s.texture.get())->texture_id; // 类型一定是这个
        glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, s.min_filter);
        glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, s.mag_filter);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, s.warp_mode);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, s.warp_mode);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, s.warp_mode);

        glActiveTexture(GL_TEXTURE0 + s.binding_id);
        s.texture->bind(s.binding_id);
    }
    checkError();
}
} // namespace Graphics
}