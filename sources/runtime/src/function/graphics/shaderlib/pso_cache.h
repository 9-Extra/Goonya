#pragma once

#include "resource/resources.h"
#include "function/graphics/opengl_utils.h"
#include "shaderlib.h"

namespace Goonya {
namespace Graphics {

struct PipelineStateContainer {
    bool enable_cilp;            // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode;       // glCullFace
    GLenum front_face_clockwise; // glFrontFace

    bool enable_depth_test;
    GLenum depth_func;

    GLuint shaderprogram_id;
};

struct PipelineStateObject {
    uint32_t id;
};

class PSOCache {
public:
    ShaderLib shader_lib;

    PipelineStateObject query_pso(const Resource::PSODesc &desc) {
        auto iter = pso_cache.find(desc);
        if (iter != pso_cache.end()) {
            return iter->second;
        } else {
            containers.emplace_back(load_pso(desc));
            uint32_t id = containers.size() - 1;
            return pso_cache.emplace(desc, id).first->second;
        }
    };

    void drop() {
        pso_cache.clear();
        containers.clear();
        shader_lib.drop();
    }

    void bind_pipeline_object(const PipelineStateObject &pso) const noexcept;

private:
    PipelineStateContainer load_pso(const Resource::PSODesc &desc);

    std::unordered_map<Resource::PSODesc, PipelineStateObject> pso_cache;
    std::vector<PipelineStateContainer> containers;
};

} // namespace Graphics
} // namespace Goonya
