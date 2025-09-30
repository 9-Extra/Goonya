#pragma once
#include "core/cgmath.h"
#include "function/renderer/passes/Passes.h"
#include "platform/graphics/Buffer.h"

namespace Goonya::Graphics {

class GeometryPass : public Pass {
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

    Ref<Buffer> per_frame_uniform; // 用于一般渲染每帧变化的数据
public:
    GeometryPass();

    void run(CameraRenderProxy *camera) override;
};

} // namespace Goonya::Graphics