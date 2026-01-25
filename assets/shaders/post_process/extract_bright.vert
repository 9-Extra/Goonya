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

layout(binding = 0) uniform sampler2D source;

out VS_OUT
{
    vec2 uv;
} vs_out;

void main()
{
    vs_out.uv = uv[index[gl_VertexID]];
    gl_Position = vec4(position[index[gl_VertexID]], 1.0f);
}