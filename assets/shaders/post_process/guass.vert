#version 440 core

#pragma GYA_INJECT

const vec3 position[4] = vec3[4](
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(-1.0f, 1.0f, -1.0f),
    vec3(1.0f, 1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f)
);

const vec2 uv[4] = vec2[4](
    vec2(0.0f, 0.0f),
    vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 0.0f)
);

const uint index[6] = uint[6](
    0, 1, 2,
    2, 3, 0
);

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

out VS_OUT
{
    vec2 uv[5];
} vs_out;

void main()
{
    vec2 origin_uv = uv[index[gl_VertexID]];
    vec2 texel_size = vec2(1.0f) * blur_size / textureSize(source, 0);
#if defined HORIZONTAL
    for(int i = 0;i < 5;i++){
        vs_out.uv[i] = vec2(origin_uv.x + texel_size.x * (i - 2), origin_uv.y);
    }
#elif defined VERTICAL
    for(int i = 0;i < 5;i++){
        vs_out.uv[i] = vec2(origin_uv.x, origin_uv.y + texel_size.y * (i - 2));
    }
#endif
    gl_Position = vec4(position[index[gl_VertexID]], 1.0f);
}