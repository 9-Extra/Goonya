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

layout(binding = 2) uniform per_material
{
    float blur_size;
};

layout(binding = 0) uniform sampler2D source;

in VS_OUT
{
    vec2 uv[5];
} vs_out;

layout(location = 0) out vec4 frag_color;

void main()
{
    const float weight[5] = float[5](0.0545, 0.2442, 0.4028, 0.2442, 0.0545);
    vec3 sum = vec3(0);
    for(int i = 0;i < 5;i++){
        vec3 pixel = textureLod(source, vs_out.uv[i], 0).rgb;
        sum += pixel * weight[i];
    }
    frag_color = vec4(sum, 1.0f);
}