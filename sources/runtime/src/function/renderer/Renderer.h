#pragma once

#include "RenderAspect.h"
#include "core/cgmath.h"
#include "core/hash_helper.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RendererBasic.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/SkyboxPass.h"
#include "passes/Passes.h"

#include <cassert>
#include <memory>
#include <unordered_set>
#include <vector>

namespace Goonya::Graphics {
// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    const CameraRenderProxy *current_camera;         // 当前正在绘制的相机
    std::unordered_set<CameraRenderProxy *> cameras; // 所有需要绘制的相机

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    std::vector<PointLight> pointlights;            // 点光源

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    std::vector<Skybox> current_skyboxs; // 天空盒

    std::unordered_set<std::unique_ptr<MeshRenderProxy>, PointerHash, PointerEqual> mesh_proxys; // 要渲染的网格

private:
    // passes
    std::unique_ptr<GeometryPass> geometry_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;

public:
    void init();

    void add_mesh_proxy(std::unique_ptr<MeshRenderProxy>&& proxy) {
        ASSERT_RENDER_THREAD();
        mesh_proxys.emplace(std::move(proxy));
    }
    void remove_mesh_proxy(MeshRenderProxy *proxy) {
        ASSERT_RENDER_THREAD();
        mesh_proxys.erase(mesh_proxys.find(proxy));
    }

    void render();

    void clear();
};

extern Renderer renderer;
} // namespace Goonya::Graphics
