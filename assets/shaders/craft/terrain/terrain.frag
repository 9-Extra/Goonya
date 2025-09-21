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
    vec3 ambient_light; //3 * 4 + 4
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    float time;
    uint pointlight_num; // 4
    PointLight pointlight_list[POINTLIGNT_MAX];
};

struct PerSurface{
    uint basecolor_id;
    vec3 normal;
};

layout(binding = 2, std430) buffer per_surface{
    PerSurface surfaces[]; // 使用SSBO实现动态大小
};


layout(binding = 0) uniform sampler2DArray basecolor_texture;


in VS_OUT
{
    vec3 world_position;
    vec2 tex_coords;
    flat uint object_id;
} vs_out;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    PerSurface surface = surfaces[vs_out.object_id];
    vec4 color = texture(basecolor_texture, vec3(vs_out.tex_coords, surface.basecolor_id));
    out_color = vec4(pow(color.xyz, vec3(1 / 2.2)), 1.0f);
    // out_color = vec4(1, 0, 0, 1);
}