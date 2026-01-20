#pragma once

#include "core/sparse_set.h"
#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderScene.h"

#include <algorithm>

#include <memory>
#include <vector>

namespace Goonya {

class Pipeline;

// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    std::vector<std::unique_ptr<CameraRenderProxy>> camera_set; // 所有的相机
    SparseSet<RenderScene> scene_set;

    bool draw_bloom = true;

private:
    std::unique_ptr<Pipeline> render_pipeline;

public:
    Handle<RenderScene> create_scene() { return scene_set.emplace(); }
    void drop_scene(Handle<RenderScene> handle) { scene_set.remove(handle); }

    RenderScene *get_scene(Handle<RenderScene> handle) { return scene_set.get_or_null(handle); }
    CameraRenderProxy *create_camera() {
        CameraRenderProxy *camera = new CameraRenderProxy();
        camera_set.emplace_back(camera);
        return camera;
    }

    void drop_camera(CameraRenderProxy *camera) {
        auto iter = std::ranges::find_if(camera_set, [camera](auto &&c) { return c.get() == camera; });
        GN_ASSERT(iter != camera_set.end());

        camera_set.erase(iter);
    }

    void init();
    void render();
    void clear();
};

extern Renderer renderer;
} // namespace Goonya
