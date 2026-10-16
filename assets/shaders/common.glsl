#ifndef COMMON_HEAD
#define COMMON_HEAD

#define POINTLIGNT_MAX 8
#define PI 3.1415926535897932384626433832795

#ifndef COMPUTE_SHADER

#ifndef VS_OUT_PS_IN
#if defined(VERTEX_SHADER)
# define VS_OUT_PS_IN out
#elif defined(FRAGMENT_SHADER)
# define VS_OUT_PS_IN in
#endif
#endif // VS_OUT_PS_IN

struct PointLight{
    vec3 position;
    vec3 intensity;
};

// --------------------顶点属性-------------------------
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

// --------------------Uniform块-----------------------

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_matrix; //16 * 4
    mat4 view_matrix_inv;
    mat4 perspective_matrix;
    mat4 view_perspective_matrix; //16 * 4
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    float time;
    vec2 screen_size;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 1) uniform per_object
{
    mat4 model_matrix;
    mat3 normal_matrix;
};

// --------------------引擎纹理-----------------------
/*
引擎纹理由引擎在渲染每一趟时自动绑定，纹理单元0..7为引擎保留，着色器使用即可
*/
layout(binding = 0) uniform sampler2D brdf_lut;
layout(binding = 1) uniform samplerCube prefiltered_envmap;
layout(binding = 2) uniform samplerCube skybox_specular_texture;
layout(binding = 3) uniform sampler2D camera_depth;

#endif // not defined COMPUTE_SHADER

#endif // COMMON_HEAD