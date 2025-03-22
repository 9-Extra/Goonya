#pragma once

#include <cassert>
#include <cmath>
#include <cstdlib>

#include "../Material.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Shader.h"
#include "platform/graphics/opengl/GLBuffer.h"

namespace Goonya {
namespace Graphics {

class GLMaterial : public Material {
public:
    GLMaterial(UberShader* uber_shader) : Material(uber_shader) {
        per_material = intrusive_ptr<GLBuffer>(uber_shader->per_material.layout.size, BufferType::DYNAMIC);
    }

    virtual void bind() override;
    virtual void update() override;

private:
    void update_shader_variant();
    void update_parameter();
    void set_pipeline_state() const noexcept;

};

} // namespace Graphics

} // namespace Goonya