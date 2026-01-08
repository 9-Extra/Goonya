#version 440 core

#define POINTLIGNT_MAX 8

#pragma GYA_INJECT

struct PointLight{
    vec3 position;
    vec3 intensity;
};

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix; //16 * 4
    mat4 view_matrix; //16 * 4
    mat4 view_matrix_inv;
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    float time;
    vec2 screen_size;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

in VS_OUT
{
    vec3 model_position;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

// ----------------------------------------------------------------------

void main()
{
    vec3 color = vec3(0, 0, 0);
    out_color = vec4(color, 1.0f);
    // out_color = vec4(abs(surface.normal), 1);
}