#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include <string>

namespace Goonya {
namespace Graphics {

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f uv;
};

struct UberShaderDesc {
    std::string vs_path;
    std::string ps_path;
};

class Shader: public intrusive_ptr_base<Shader>{
public:
    virtual void bind() = 0;
    virtual ~Shader() = default;
};

class PipelineStateObject: public intrusive_ptr_base<PipelineStateObject> {
public:
    virtual void bind() const = 0;
    virtual ~PipelineStateObject() = default;
protected:
    PipelineStateObject() {};
};

class Material: public intrusive_ptr_base<Material>{
public:
    virtual void bind() const = 0;
    virtual ~Material() = default;
protected:
    Material() {};
};

}
}