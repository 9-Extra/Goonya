#pragma once

#include "../RenderResource.h"
#include "core/intrusive_ptr.h"
#include "platform/graphics/Buffer.h"
#include "platform/graphics/graphics.h"

namespace Goonya {
namespace Graphics {
// Pass 基类
class Pass {
public:
    virtual void run() = 0;
    friend class Renderer;

    virtual ~Pass() = default;
};

class LambertianPass : public Pass {
public:
    LambertianPass();
    void reset() {}

    virtual void run() override;

private:
    static constexpr unsigned int POINTLIGNT_MAX = 8;
    struct PointLightData final {
        alignas(16) Vector3f position;
        alignas(16) Vector3f intensity;
    };
    struct PerFrameData final {
        Matrix4 view_perspective_matrix;
        alignas(16) Vector3f ambient_light;
        alignas(16) Vector3f camera_position;
        alignas(4) float fog_min_distance;
        alignas(4) float fog_density;
        alignas(4) uint32_t pointlight_num;
        PointLightData pointlight_list[POINTLIGNT_MAX];
    };

    struct PerObjectData final {
        Matrix4 model_matrix;
        Matrix4 normal_matrix;
    };

    intrusive_ptr<Buffer> per_frame_uniform;  // 用于一般渲染每帧变化的数据
    intrusive_ptr<Buffer> per_object_uniform; // 用于一般渲染每个物体不同的数据
};

class SkyBoxPass : public Pass {
public:
    SkyBoxPass()
        : skybox_uniform(graphics_api->create_buffer(sizeof(SkyBoxData), BufferType::DYNAMIC)),
          mesh(resources.meshes.at("skybox_cube")) {}

    virtual void run() override;

private:
    const static unsigned int SKYBOX_TEXTURE_BINDIGN = 5;
    struct SkyBoxData final {
        Matrix4 skybox_view_perspective_matrix;
    };

    intrusive_ptr<Buffer> skybox_uniform;
    intrusive_ptr<Mesh> mesh;
};

} // namespace Graphics
} // namespace Goonya