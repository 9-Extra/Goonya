#pragma once

#include "RenderAspect.h"
#include "core/cgmath.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/RendererBasic.h"
#include "passes/Passes.h"

#include <cassert>
#include <functional>
#include <memory>
#include <queue>
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

    std::unordered_set<MeshRenderProxy *> meshes; // 要渲染的网格

private:
    // passes
    std::unique_ptr<LambertianPass> lambertian_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;

    std::queue<std::function<void()>> render_tasks;

public:
    void init();

    void add_mesh_proxy(MeshRenderProxy *proxy) {
        ASSERT_RENDER_THREAD();
        meshes.emplace(proxy);
    }
    void remove_mesh_proxy(MeshRenderProxy *proxy) {
        ASSERT_RENDER_THREAD();
        meshes.erase(meshes.find(proxy));
    }

    template <typename T>
    void enqueue_render_task(T &&task) {
        render_tasks.push(std::forward<T>(task));
    }

    void run_all_tasks();
    void render();

    void clear();
};

extern Renderer renderer;
} // namespace Goonya::Graphics
