#pragma once

#include "platform/graphics/Mesh.h"
#include <vector>

#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLMesh final : public Mesh {
public:
    GLMesh(Topology topology, VertexLayout layout, intrusive_ptr<Buffer> vertex_buffers,
           intrusive_ptr<Buffer> index_buffer);

    GLMesh(const std::vector<SubMesh> &submeshes, VertexLayout layout, intrusive_ptr<Buffer> vertex_buffers,
           intrusive_ptr<Buffer> index_buffer);

    virtual ~GLMesh() { glDeleteVertexArrays(1, &vao_id); }

    // ------------------------------------
    virtual void bind() const noexcept override { glBindVertexArray(vao_id); }

protected:
    virtual void _set_debug_label(const std::string &name) const noexcept override {
        glObjectLabel(GL_VERTEX_ARRAY, vao_id, name.size(), name.data());
    }

private:
    GLuint vao_id;
};

} // namespace Graphics

} // namespace Goonya