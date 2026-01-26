#version 440 core

#pragma GYA_INJECT

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

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

layout(binding = 1) uniform per_object
{
    mat4 model_matrix;
    mat3 normal_matrix;
};

out VS_OUT
{
    vec3 normal;
    vec4 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

void main()
{
    vec4 world_position = vec4(position.xyz, 1.0f) * model_matrix;
    vs_out.world_position = world_position.xyz / world_position.w;
    vs_out.normal = normal * normal_matrix;
    vs_out.tangent = vec4(tangent.xyz * mat3(model_matrix), tangent.w);
    vs_out.tex_coords = uv;

    gl_Position = world_position * view_perspective_matrix;
}