#pragma once

#include "shaderlib.h"

namespace Goonya {
namespace Graphics {

struct PSODesc {
    ShaderDesc shader_desc;

    bool enable_cilp = true;    // glEnable(GL_CULL_FACE)
    GLenum cull_face_mode = GL_BACK; // glCullFace
    GLenum front_face_clockwise = GL_CCW; // glFrontFace

    bool enable_depth_test = true; // glEnable(GL_DEPTH_TEST)
    GLenum depth_func = GL_LESS; // glDepthFunc

    bool operator==(const PSODesc &b) const noexcept = default;
};

struct PSOHasher {
    size_t operator()(const PSODesc &desc) const noexcept{
        return std::hash<decltype(desc.shader_desc)>{}(desc.shader_desc) & desc.cull_face_mode &
               ((size_t)desc.cull_face_mode << 32) & desc.enable_cilp & desc.depth_func & desc.enable_depth_test << 1;
    }
};

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

    PipelineStateObject query_pso(const PSODesc &desc) {
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
    PipelineStateContainer load_pso(const PSODesc &desc) {
        PipelineStateContainer container{.enable_cilp = desc.enable_cilp,
                                         .cull_face_mode = desc.cull_face_mode,
                                         .front_face_clockwise = desc.front_face_clockwise,
                                         .enable_depth_test = desc.enable_depth_test,
                                         .depth_func = desc.depth_func,
                                         .shaderprogram_id = shader_lib.query_shader(desc.shader_desc).gl_id};
        return container;
    };

    std::unordered_map<PSODesc, PipelineStateObject, PSOHasher> pso_cache;
    std::vector<PipelineStateContainer> containers;
};

} // namespace Graphics
} // namespace Goonya
