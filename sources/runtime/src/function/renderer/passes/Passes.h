#pragma once

#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/Graphics.h"
#include "resource/Resource.h"

namespace Goonya::Graphics {
// Pass 基类
class Pass {
public:
    virtual void run() = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

class LambertianPass : public Pass {
private:
    static constexpr unsigned int POINTLIGHT_MAX = 8;
    struct PointLightData final {
        alignas(16) Vector3f position;
        alignas(16) Vector3f intensity;
    };
    struct PerFrameData final { // NOLINT：不需要初始化
        Matrix4 view_perspective_matrix;
        alignas(16) Vector3f ambient_light;
        alignas(16) Vector3f camera_position;
        alignas(4) float fog_min_distance;
        alignas(4) float fog_density;
        alignas(4) float time;
        alignas(4) uint32_t pointlight_num;
        PointLightData pointlight_list[POINTLIGHT_MAX];
    };

    struct PerObjectData final {
        Matrix4 model_matrix;
        Matrix4 normal_matrix;
    };

    intrusive_ptr<Buffer> per_frame_uniform; // 用于一般渲染每帧变化的数据
    intrusive_ptr<Buffer> per_object_uniform; // 用于一般渲染每帧变化的数据
public:
    LambertianPass();
    void reset() {}

    void run() override;
};

class SkyBoxPass : public Pass {
private:
    const static unsigned int SKYBOX_TEXTURE_BINDING = 5;
    struct SkyBoxData final {
        Matrix4 skybox_view_perspective_matrix;
    };

    intrusive_ptr<Buffer> skybox_uniform;
    intrusive_ptr<Mesh> mesh;
public:
    SkyBoxPass()
        : skybox_uniform(graphics_api->create_buffer(sizeof(SkyBoxData), BufferType::DYNAMIC)),
          mesh(Resource::resources.meshes.get("skybox_cube")) {}

    void run() override;
};

} // namespace Goonya::Graphics
