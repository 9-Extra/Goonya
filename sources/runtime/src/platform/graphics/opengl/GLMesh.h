#pragma once

#include "platform/graphics/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "resource/resources.h"

namespace Goonya {
namespace Graphics {

class GLMesh : public Mesh {
public:
    virtual void bind() const override {
        glBindVertexArray(vao_id);
    }

    virtual uint32_t get_indices_count() override { return index_buffer->get_index_count(); }

    virtual ~GLMesh() { glDeleteVertexArrays(1, &vao_id); }

private:
    GLenum topology;
    Resource::VertexLayout layout;
    intrusive_ptr<GLVertexBuffer> vertex_buffer;
    intrusive_ptr<GLIndexBuffer> index_buffer;
    GLuint vao_id;

    friend class OpenGLGraphicsAPI;
    GLMesh(GLenum topology, Resource::VertexLayout layout, intrusive_ptr<GLVertexBuffer> vertex_buffers,
           intrusive_ptr<GLIndexBuffer> index_buffer);
};

} // namespace Graphics

} // namespace Goonya