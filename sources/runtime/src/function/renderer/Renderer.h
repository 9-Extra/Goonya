#pragma once

#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderScene.h"
#include "function/renderer/passes/GeometryPass.h"
#include "function/renderer/passes/SkyboxPass.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace Goonya::Graphics {
// 渲染管理器，包含所有渲染需要的数据供pass使用, 在world tick时各种组件会将渲染数据写到这里
class Renderer final {
public:
    std::vector<std::unique_ptr<CameraRenderProxy>> camera_set; // 所有的相机
    std::vector<std::unique_ptr<RenderScene>> scene_set;
private:
// passes
    std::unique_ptr<GeometryPass> geometry_pass;
    std::unique_ptr<SkyBoxPass> skybox_pass;
public: 
    RenderScene* create_scene() {
        RenderScene* scene = new RenderScene();
        scene_set.emplace_back(scene);
        return scene;
    }
    void drop_scene(RenderScene* scene){
        auto iter = std::ranges::find_if(scene_set, [scene](auto&& a){return a.get() == scene;});
        assert(iter != scene_set.end());

        scene_set.erase(iter); 
    }
    CameraRenderProxy* create_camera() {
        CameraRenderProxy* camera = new CameraRenderProxy();
        camera_set.emplace_back(camera);
        return camera;
    }

    void drop_camera(CameraRenderProxy* camera){
        auto iter = std::ranges::find_if(camera_set, [camera](auto&& c){return c.get() == camera;});
        assert(iter != camera_set.end());

        camera_set.erase(iter); 
    }

    void init();
    void render();
    void clear();
};

extern Renderer renderer;
} // namespace Goonya::Graphics
