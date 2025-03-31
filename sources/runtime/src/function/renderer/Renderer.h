#pragma once

#include "RenderAspect.h"
#include "core/cgmath.h"
#include "passes/Passes.h"
#include <memory>
#include <unordered_set>
#include <vector>


namespace Goonya {
namespace Graphics {
// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    const CameraRenderInfo* current_camera;  // 当前正在绘制的相机
    std::unordered_set<CameraRenderInfo *> cameras; // 所有需要绘制的相机

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    std::vector<PointLight> pointlights;            // 点光源

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    std::vector<Skybox> current_skyboxs; // 天空盒

    std::unordered_set<MeshRenderInfo *> meshes; // 要渲染的网格

    void init();

    void add_mesh_info(MeshRenderInfo *info) { meshes.emplace(info); }

    void remove_mesh_info(MeshRenderInfo *info) { meshes.erase(meshes.find(info)); }

    void update_mesh_transform(MeshRenderInfo *info, const Matrix4 &model_matrxi, const Matrix3 &normal_matrix) {
        info->model_matrix = model_matrxi;
        info->normal_matrix = normal_matrix;
    }

    void render();

    void clear() {
        current_skyboxs.clear();

        lambertian_pass.reset();
        skybox_pass.reset();
    }

private:
    // passes
    std::unique_ptr<LambertianPass> lambertian_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;
};

extern Renderer renderer;
} // namespace Graphics
} // namespace Goonya