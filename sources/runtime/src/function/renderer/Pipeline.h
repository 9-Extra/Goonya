#pragma once

#include "function/renderer/RenderProxy/Camera.h"
#include "function/renderer/RenderScene.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLRenderTarget.h"

namespace Goonya {

static constexpr unsigned int POINTLIGHT_MAX = 8;

struct PointLightData final {
    alignas(16) Vector3f position;
    alignas(16) Vector3f intensity;
};

struct PerFrameData final { // NOLINT：不需要初始化
    Matrix4f view_perspective_matrix;
    Matrix4f view_matrix;
    Matrix4f view_matrix_inv;
    alignas(16) Vector3f ambient_light;
    alignas(16) Vector3f camera_position;
    alignas(4) float fog_min_distance;
    alignas(4) float fog_density;
    alignas(4) float time;
    alignas(4) Vector2f screen_size;
    alignas(4) uint32_t pointlight_num;
    PointLightData pointlight_list[POINTLIGHT_MAX];
};

constexpr uint32_t PER_FRAME_UNIFORM_BINDING = 0;

struct alignas(256) PerObjectData final {
    Matrix4f model_matrix;
    Matrix4f normal_matrix; // 内存对齐
};

constexpr uint32_t PER_OBJECT_UNIFORM_BINDING = 1;

class Pipeline {
private:
    struct RenderContext {
        Ref<RenderTarget> render_target;
        CameraRenderProxy *camera;
        RenderScene *scene;
        float aspect_ratio;
        Ref<GLTexture> env_map;
        Ref<Material> skybox_material;
    };

    Ref<GLFrameBuffer> replace_render_target[2]; // 替换渲染目标，用于渲染到屏幕，两个目标用于PingPong
    const static unsigned int SKYBOX_TEXTURE_BINDING = 5;
    Ref<GLMesh> skybox_mesh;
    Ref<GLMesh> postprocess_quad_mesh;
    Ref<Material> postprocess_material;

    Ref<Material> guassian_blur_material_horizontal;
    Ref<Material> guassian_blur_material_vertical;
    Ref<Material> bright_extract_material;
    Ref<GLFrameBuffer> bloom_render_target[2];

public:
    Pipeline();
    virtual ~Pipeline() = default;

    virtual void render();

private:
    void render_camera(RenderContext &context);

    void draw_geometry(RenderContext &context);
    void draw_skybox(RenderContext &context);
    void draw_postprocess(RenderContext &context);

    void screen_paint(Ref<GLFrameBuffer> dst, Ref<GLFrameBuffer> src, Ref<Material> material);
};

} // namespace Goonya