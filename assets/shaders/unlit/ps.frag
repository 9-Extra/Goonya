#version 440 core

#pragma GYA_INJECT

layout(binding = 2) uniform per_material
{
    vec3 emissive;
};

uniform sampler2D emissive_texture;

in VS_OUT
{
    vec2 tex_coords;
} fs_in;

out vec4 out_color; // 片段着色器输出的变量名可以任意命名，类型必须是vec4

void main()
{
    out_color = texture(emissive_texture, fs_in.tex_coords) * vec4(emissive, 1.0);
}