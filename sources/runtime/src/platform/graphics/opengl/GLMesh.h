#pragma once

#include "core/RefCount.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Mesh.h"

#include <cassert>
#include <cstdint>
#include <glad/glad.h>
#include <span>

namespace Goonya::Graphics {

class GLMesh final : public Mesh {
private:
    GLuint vao_id = 0;

    Ref<Buffer> indices_buffer;
    std::array<Ref<Buffer>, 16> vertices_buffers{};
public:
    explicit GLMesh(VertexLayout layout) noexcept;

    ~GLMesh() override { glDeleteVertexArrays(1, &vao_id); }

    // ------------------------------------
    void bind() const noexcept override {
        glBindVertexArray(vao_id);
    }

    void set_vertices(uint32_t stream_id, const std::span<const std::byte> &data) noexcept override;
    void set_indices(const std::span<const uint32_t> &indices) noexcept override;

protected:
    void _set_debug_label(const std::string &name) const noexcept override {
        glObjectLabel(GL_VERTEX_ARRAY, vao_id, (GLsizei)name.size(), name.data());
    }
};

} // namespace Goonya::Graphics
