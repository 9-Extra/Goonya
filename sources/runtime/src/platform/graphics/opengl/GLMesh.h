#pragma once

#include "platform/graphics/Mesh.h"
#include "platform/graphics/opengl/GLBuffer.h"
#include "resource/resources.h"
#include <cstdint>
#include <vector>

namespace Goonya {
namespace Graphics {

struct GLSubMesh{
    uint32_t start_index;
    uint32_t index_count;
    GLenum topology;
};

class GLMesh : public Mesh {
public:
    virtual uint32_t get_submesh_count() const noexcept override{
        return (uint32_t)submeshes.size();
    }
    virtual ~GLMesh() { glDeleteVertexArrays(1, &vao_id); }
    
    // ------------------------------------
    void bind() const noexcept {
        glBindVertexArray(vao_id);
    }

private:
    std::vector<GLSubMesh> submeshes;

    Resource::VertexLayout layout;
    intrusive_ptr<GLVertexBuffer> vertex_buffer;
    intrusive_ptr<GLIndexBuffer> index_buffer;
    GLuint vao_id;

    friend class OpenGLGraphicsAPI;
    GLMesh(Topology topology, Resource::VertexLayout layout, intrusive_ptr<GLVertexBuffer> vertex_buffers,
           intrusive_ptr<GLIndexBuffer> index_buffer);

    GLMesh(const std::vector<SubMesh>& submeshes, Resource::VertexLayout layout, intrusive_ptr<GLVertexBuffer> vertex_buffers,
    intrusive_ptr<GLIndexBuffer> index_buffer);
};

} // namespace Graphics

} // namespace Goonya