#version 440 core

#pragma GYA_INJECT

#define POINTLIGNT_MAX 8

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

layout(binding = 0) uniform sampler2D source;

in VS_OUT
{
    vec2 uv;
} vs_out;

layout(location = 0) out vec4 frag_color;

vec4 post_process(vec3 color_in){
    return vec4(pow(color_in, vec3(1 / 2.2)), 1.0f);
}

void main()
{
    vec3 pixel = textureLod(source, vs_out.uv, 0).rgb;
    frag_color = post_process(pixel);
}