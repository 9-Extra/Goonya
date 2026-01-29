#name basic_post_process
#setting depth_test="off"
#setting cull_mode="off"

#texture source="buildin:missing_texture"
#texture bloom="buildin:missing_texture"

#local_variant _ BLOOM

#section common

#include "post_process.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec2 uv;
} vs_out;

#section vertex

void vert()
{
    vs_out.uv = uv[index[gl_VertexID]];
    gl_Position = vec4(position[index[gl_VertexID]], 1.0f);
}

#section fragment

layout(location = 0) out vec4 out_color;

uniform sampler2D source;
uniform sampler2D bloom;

vec3 color_encode(vec3 color_in){
    return pow(color_in, vec3(1 / 2.2));
}
void frag()
{
    vec3 pixel = textureLod(source, vs_out.uv, 0).rgb;
#if defined(BLOOM)    
    vec3 bloom_pixel = textureLod(bloom, vs_out.uv, 0).rgb;
    pixel = 1 - (1 - pixel) * (1 - bloom_pixel);
#endif
    out_color = vec4(color_encode(pixel), 1.0f);
}