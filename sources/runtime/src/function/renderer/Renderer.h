#pragma once

#include <memory>
#include <unordered_set>
#include <vector>
#include "core/cgmath.h"
#include "passes/Passes.h"
#include "RenderAspect.h"

namespace Goonya {
namespace Graphics {
// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    struct Viewport {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    } main_viewport; // 主视口

    Camera main_camera;             // 主像机
    void *active_camera;            // 实际上是CpntCamera的owner的指针

    Vector3f ambient_light = {0.02f, 0.02f, 0.02f}; // 环境光
    std::vector<PointLight> pointlights;            // 点光源

    float fog_min_distance = 5.0f; // 雾开始的距离
    float fog_density = 0.001f;    // 雾强度

    std::vector<Skybox> current_skyboxs; // 天空盒

    std::unordered_set<MeshRenderInfo*> meshes; // 要渲染的网格

    void init();

    void add_mesh_info(MeshRenderInfo* info){
        meshes.emplace(info);
    }

    void remove_mesh_info(MeshRenderInfo* info){
        meshes.erase(meshes.find(info));
    }

    void update_mesh_transform(MeshRenderInfo* info, const Matrix4& model_matrxi, const Matrix4& normal_matrix){
        info->model_matrix = model_matrxi;
        info->normal_matrix = normal_matrix;
    }

    void render();

    void set_viewport(int32_t  x, int32_t  y, int32_t  width, int32_t  height) { main_viewport = {x, y, width, height}; }

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
}
}