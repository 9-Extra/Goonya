#pragma once

#include "platform/graphics/opengl/GLMaterial.h"
#include "resource/resources.h"
#include "shaderlib.h"

namespace Goonya {
namespace Graphics {

class PSOCache {
public:
    void add_uber_shader(const std::string &name, const UberShaderDesc &desc) {
        shader_lib.add_uber_shader(name, desc);
    }
    
    intrusive_ptr<GLPipelineStateObject> query_pso(const Resource::PSODesc &desc) {
        auto iter = pso_cache.find(desc);
        if (iter != pso_cache.end()) {
            return iter->second;
        } else {
            auto r = pso_cache.emplace(desc, load_pso(desc));
            return r.first->second;
        }
    };
    
private:
    friend class GLPipelineStateObject;
    ShaderLib shader_lib;
    std::unordered_map<Resource::PSODesc, intrusive_ptr<GLPipelineStateObject>> pso_cache;
    
    intrusive_ptr<GLPipelineStateObject> load_pso(const Resource::PSODesc &desc){
        return intrusive_ptr<GLPipelineStateObject>{desc};
    }
};

} // namespace Graphics
} // namespace Goonya
