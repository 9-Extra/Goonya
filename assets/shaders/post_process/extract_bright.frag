#version 440 core

#pragma GYA_INJECT

layout(binding = 2) uniform per_material
{
    float threshold;
    float intensity;
};

layout(binding = 0) uniform sampler2D source;

in VS_OUT
{
    vec2 uv;
} vs_out;

layout(location = 0) out vec4 frag_color;

void main()
{
    const float knee = 0.2f;
    vec3 color = textureLod(source, vs_out.uv, 0).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float contribute = max(0.0f, luminance - threshold);
    
    contribute = contribute / (contribute + knee);

    frag_color = vec4(color * contribute * intensity, 1.0f);
}