#ifndef COMMON_HEAD
#define COMMON_HEAD

#define POINTLIGNT_MAX 8

struct PointLight{
    vec3 position;
    vec3 intensity;
};

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

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

#if defined(VERTEX_SHADER)
# define VS_OUT_PS_IN out
#elif defined(FRAGMENT_SHADER)
# define VS_OUT_PS_IN in
#endif

#endif