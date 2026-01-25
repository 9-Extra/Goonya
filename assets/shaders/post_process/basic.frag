#version 440 core

#pragma GYA_INJECT

layout(binding = 0) uniform sampler2D source;
layout(binding = 1) uniform sampler2D bloom;

in VS_OUT
{
    vec2 uv;
} vs_out;

layout(location = 0) out vec4 frag_color;

vec3 color_encode(vec3 color_in){
    return pow(color_in, vec3(1 / 2.2));
}

void main()
{
    vec3 pixel = textureLod(source, vs_out.uv, 0).rgb;
#if defined(BLOOM)    
    vec3 bloom_pixel = textureLod(bloom, vs_out.uv, 0).rgb;
    pixel = 1 - (1 - pixel) * (1 - bloom_pixel);
#endif
    frag_color = vec4(color_encode(pixel), 1.0f);
}