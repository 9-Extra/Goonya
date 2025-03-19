#pragma once

#include <cassert>
#include <cmath>
#include <cstdlib>

#include "../Material.h"
#include "core/intrusive_ptr.h"
#include "GLShader.h"

namespace Goonya {
namespace Graphics {

class GLMaterial : public Material {
public:
    GLMaterial(const PSODesc &pso) : Material(pso) {}

    virtual void bind() override;
    virtual void update() override;

private:
    void reset_pso();

    void update_parameter();

    const GLShader* get_shader() const {
        auto pso = dynamic_cast<GLPipelineStateObject *>(this->pso.get());
        assert(pso);
        return pso->shader.get();
    }
};

} // namespace Graphics

} // namespace Goonya