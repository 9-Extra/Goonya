#pragma once

#include "core/cgmath/cgmath.h"
#include "core/hash_helper.h"
#include "core/plf_colony.h"
#include "function/renderer/RenderAspect.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include <cassert>
#include <memory>
#include <unordered_set>

namespace Goonya {

class RenderScene {
public:
    // -----------------环境-------------------
    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    plf::colony<PointLight> pointlights;            // 点光源
    
    plf::colony<Skybox> skyboxs; // 天空盒

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度
    // -----------------物体-------------------
    std::unordered_set<std::unique_ptr<MeshRenderProxy>, PointerHash, PointerEqual> mesh_proxys; // 要渲染的网格

    public:
    void clear() /*NOLINT*/ {
        assert(pointlights.empty());
        assert(mesh_proxys.empty());
        assert(skyboxs.empty());
    }

private:
    friend class Renderer;
    RenderScene() = default;

};

} // namespace Goonya
