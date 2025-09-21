#version 440 core

#pragma GYA_INJECT

layout (location = 0) in vec3 position;
layout (location = 4) in vec2 uv;

#define POINTLIGNT_MAX 8

struct PointLight{
    vec3 position;
    vec3 intensity;
};

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix; //16 * 4
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    float time;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

layout(binding = 1) uniform per_object
{
    mat4 model_matrix;
    mat4 normal_matrix;
};

out VS_OUT
{
    vec3 world_position;
    vec2 tex_coords;
    flat uint object_id;
} vs_out;

void main()
{
    uint object_id = gl_VertexID / 6;
    vs_out.world_position = position;
    vs_out.tex_coords = uv;
    vs_out.object_id = object_id;

    gl_Position = vec4(position, 1) * view_perspective_matrix;
}