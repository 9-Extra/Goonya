#version 440 core

#pragma GYA_INJECT

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 color;
layout (location = 4) in vec2 uv;

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
};

layout(binding = 1, std140) uniform per_object
{
    mat4 model_matrix;
};

out VS_OUT
{
    vec2 tex_coords;
} vs_out;

void main()
{
    vs_out.tex_coords = uv;
    gl_Position = vec4(position.xyz, 1.0f) * model_matrix * view_perspective_matrix;
}