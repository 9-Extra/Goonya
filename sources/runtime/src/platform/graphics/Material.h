#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Texture.h"
#include "resource/resources.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

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

class Shader : public intrusive_ptr_base<Shader> {
public:
    virtual void bind() = 0;
    virtual ~Shader() = default;
};

class PipelineStateObject : public intrusive_ptr_base<PipelineStateObject> {
public:
    virtual void bind() const = 0;
    virtual ~PipelineStateObject() = default;

protected:
    PipelineStateObject() {};
};

class Material : public intrusive_ptr_base<Material> {
public:
    virtual void bind() = 0;
    virtual void update() = 0;

    virtual ~Material() = default;

    void set_param(const std::string &name, const Meta::DynamicData &value) {
        if (auto iter = parameters.find(name); iter != parameters.end()) {
            if (iter->second != value){
                iter->second = value;
                is_parameters_dirty = true;
            }
        } else {
            parameters.emplace(name, value);
            is_parameters_dirty = true;
        }
    }

    void set_texture(const std::string &name, intrusive_ptr<Texture> texture) {
        texture_info[name] = texture;
    }

protected:
    Material(const Resource::PSODesc &pso) : pso_desc(pso), is_parameters_dirty(true), is_pso_dirty(true) {};
    
    Resource::PSODesc pso_desc;
    
    // 所有参数在内存中保存一份
    std::unordered_map<std::string, Meta::DynamicData> parameters; // 只保存参数，无论它是否被着色器需要
    std::unordered_map<std::string, intrusive_ptr<Texture>> texture_info;     // 需要的纹理，sampler-name -> texture
    
    // 脏标记
    mutable bool is_parameters_dirty;
    mutable bool is_pso_dirty;

    // 设备侧
    intrusive_ptr<PipelineStateObject> pso;
    intrusive_ptr<UniformBuffer> per_material;                     // 显存中的材质参数
    std::unordered_map<uint32_t, intrusive_ptr<Texture>> textures; // slot -> texture
};

} // namespace Graphics
} // namespace Goonya