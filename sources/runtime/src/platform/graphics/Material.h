#pragma once

#include "core/intrusive_ptr.h"
#include "core/metatype/metatype.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Texture.h"
#include "platform/graphics/Shader.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Goonya {
namespace Graphics {

struct MaterialDesc {
    std::unordered_map<std::string, Meta::DynamicData> parameters;
    std::vector<std::tuple<std::string, std::string>> textures; // (着色器中名称, 资源键)

    PSODesc pso_desc;
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

    void set_shader_variant_key(const std::string &key) {
        std::unordered_set<std::string>& shader_def = pso_desc.shader_desc.variant_keys;
        
        if (shader_def.contains(key)) return;

        shader_def.emplace(key);
        is_pso_dirty = true;
    }

    void remove_shader_variant_key(const std::string &key) {
        std::unordered_set<std::string>& shader_def = pso_desc.shader_desc.variant_keys;
        
        if (auto iter = shader_def.find(key);iter != shader_def.end()){
            shader_def.erase(iter);
            is_pso_dirty = true;
        }
    }

protected:
    Material(const PSODesc &pso) : pso_desc(pso), is_parameters_dirty(true), is_pso_dirty(true) {};
    
    PSODesc pso_desc;
    
    // 所有参数在内存中保存一份
    std::unordered_map<std::string, Meta::DynamicData> parameters; // 只保存参数，无论它是否被着色器需要
    std::unordered_map<std::string, intrusive_ptr<Texture>> texture_info;     // 需要的纹理，sampler-name -> texture
    
    // 脏标记
    mutable bool is_parameters_dirty;
    mutable bool is_pso_dirty;

    // 设备侧
    intrusive_ptr<PipelineStateObject> pso;
    intrusive_ptr<Buffer> per_material;                     // 显存中的材质参数
    std::unordered_map<uint32_t, intrusive_ptr<Texture>> textures; // slot -> texture
};

} // namespace Graphics
} // namespace Goonya