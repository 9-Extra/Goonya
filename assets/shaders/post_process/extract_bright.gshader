#name extract_bright
#setting depth_test="off"
#setting cull_mode="off"

#texture source="buildin:missing_texture"

#param threshold=f32(0.5f)
#param intensity=f32(2.0f)

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

uniform sampler2D source;

layout(location = 0) out vec4 out_color;

void frag()
{
    const float knee = 0.2f;
    vec3 color = textureLod(source, vs_out.uv, 0).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float contribute = max(0.0f, luminance - threshold);
    
    contribute = contribute / (contribute + knee);

    out_color = vec4(color * contribute * intensity, 1.0f);
}