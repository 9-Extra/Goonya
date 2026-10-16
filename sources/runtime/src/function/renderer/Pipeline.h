#pragma once

#include "Material.h"
#include "core/cgmath/matrix.h"
#include "core/cgmath/vector.h"
#include "function/renderer/Camera.h"
#include "function/renderer/ComputeMaterial.h"
#include "function/renderer/RScene.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLRenderTarget.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "resource/Resource.h"

#include <vector>

namespace Goonya {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct CullInstance {
    Mesh *mesh;
    Material *material;
    SubMesh submesh;
    GLBuffer *per_object_uniform;
    float distance_to_camera;
    bool is_skin;
};

class Pipeline {
private:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    struct CameraInfo {
        float aspect_ratio;
        REnvironment *env;
        RCamera *camera;
        RScene *scene;

        // 冗余，但防止重复计算
        Matrix4f view_matrix;
        Matrix4f projection_matrix;
        Matrix4f view_projection_matrix;
    };

    Ref<GLFrameBuffer> replace_render_target[2]; // 替换渲染目标，用于渲染到屏幕，两个目标用于PingPong

    Ref<Mesh> skybox_mesh;
    Ref<Material> skybox_material;

    Ref<Material> depth_material;
    Ref<GLFrameBuffer> depth_fbo;
    Ref<GLTexture> depth_texture;

    // 整个引擎共用的BRDF LUT
    Ref<GLTexture> brdf_lut;

    Ref<ComputeMaterial> skinning_material;

    Ref<Mesh> postprocess_quad_mesh;
    Ref<Material> postprocess_material;

    Ref<Material> guassian_blur_material_horizontal;
    Ref<Material> guassian_blur_material_vertical;
    Ref<Material> bright_extract_material;
    Ref<GLFrameBuffer> bloom_render_target[2];

    bool texture_preview_windows_open = true;
    bool show_texture = true;
    float show_texture_mipmap_level = 0;
    Vector2f show_texture_position{0.75, 0.75};
    Vector2f show_texture_size{0.25, 0.25};
    AssetKey show_texture_key = "buildin:white";
    Ref<Material> texture_display_material;
    

public:
    Pipeline();
    virtual ~Pipeline() = default;

    virtual void render();

private:
    void render_camera(RCamera *camera, RScene *scene);

    std::vector<CullInstance> cull(CameraInfo &camera_info);

    void compute_skinning(CameraInfo &camera_info, std::vector<CullInstance> &visible_instances);

    void draw_depth(CameraInfo &camera_info, std::vector<CullInstance> &visible_instances);
    void draw_geometry(CameraInfo &camera_info, std::vector<CullInstance> &visible_instances);
    void draw_skybox(CameraInfo &camera_info);
    void draw_postprocess(CameraInfo &camera_info);

    // 把一个指定纹理画在屏幕上方便查看
    void draw_texture_preview();

    void screen_paint(Ref<GLFrameBuffer> dst, Ref<GLFrameBuffer> src, Ref<Material> material);
};

} // namespace Goonya