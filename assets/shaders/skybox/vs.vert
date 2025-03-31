#version 440 core

#pragma GYA_INJECT

layout (location = 0) in vec3 position;

layout(binding = 0, std140) uniform per_frame
{
    mat4 view_perspective_matrix;
};

out VS_OUT
{
    vec3 cube_map_texcoords;
} vs_out;

void main()
{
    vs_out.cube_map_texcoords = position.xyz;
    vec4 pos = view_perspective_matrix * vec4(position, 1.0f);
    gl_Position = pos.xyww;// 相当于z = w， 欺骗一下深度测试 https://www.jianshu.com/p/ad691b3ea9d5
}