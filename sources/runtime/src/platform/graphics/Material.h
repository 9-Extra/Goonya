#pragma once

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Texture.h"
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
    virtual void bind() const = 0;
    virtual void update_parameters() const noexcept = 0;

    virtual ~Material() = default;

    void set_param(const std::string &name, const Meta::DynamicData &value) {
        parameters.at(name) = value;
        is_dirty = true;
    }
    void set_param_if_exist(const std::string &name, const Meta::DynamicData &vaule) noexcept {
        if (auto iter = parameters.find(name); iter != parameters.end()) {
            iter->second = vaule;
            is_dirty = true;
        }
    }
    void set_texture(const std::string &name, intrusive_ptr<Texture> texture) {
        textures[texture_info.at(name)] = texture;
    }
    void set_texture_if_exist(const std::string &name, intrusive_ptr<Texture> texture) noexcept {
        if (auto iter = texture_info.find(name); iter != texture_info.end()) {
            textures[iter->second] = texture;
        }
    }

protected:
    Material(const intrusive_ptr<PipelineStateObject> &pso) : is_dirty(true), pso(pso) { assert(pso); };
    // 所有参数在内存中保存一份
    std::unordered_map<std::string, Meta::DynamicData> parameters; // 只保存真正需要的参数
    std::unordered_map<std::string, uint32_t> texture_info;        // 需要的纹理
    mutable bool is_dirty;

    // 设备侧
    intrusive_ptr<PipelineStateObject> pso;
    mutable intrusive_ptr<UniformBuffer> per_material;                     // 显存中的材质参数
    std::unordered_map<uint32_t, intrusive_ptr<Texture>> textures; // slot -> texture
};

} // namespace Graphics
} // namespace Goonya