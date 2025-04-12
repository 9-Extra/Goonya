#pragma once

#include "platform/graphics/Mesh.h"

#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLMesh final : public Mesh {
public:
    GLMesh() {
        glCreateVertexArrays(1, &vao_id); // 创建空的vao
        is_dirty = true;
    };

    virtual ~GLMesh() { glDeleteVertexArrays(1, &vao_id); }

    // ------------------------------------
    virtual void bind() const noexcept override {
        update(); 
        glBindVertexArray(vao_id); 
    }
    virtual void update() const noexcept override{
        if (is_dirty){
            update_VAO();
            is_dirty = false;
        }
    }

protected:
    virtual void _set_debug_label(const std::string &name) const noexcept override {
        glObjectLabel(GL_VERTEX_ARRAY, vao_id, name.size(), name.data());
    }

private:
    GLuint vao_id;

    void update_VAO() const noexcept;
};

} // namespace Graphics

} // namespace Goonya