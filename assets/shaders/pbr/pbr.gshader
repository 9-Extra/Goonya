#name pbr
#texture basecolor_texture="buildin:missing_texture"
#texture normal_texture="buildin:normal"
#texture orm_texture="buildin:white"
#texture prefilitered_envmap="buildin:missing_texture"
#texture brdf_lut="buildin:brdf_lut"

#param metallic_factor=f32(1)
#param roughness_factor=f32(1)

#global_variant GYA_FOG_EXP

#local_variant _ USE_NORMAL_MAP
#local_variant _ USE_ORM_TEXTURE

#section common
#include "shaders/common.glsl"

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D orm_texture;
layout(binding = 3) uniform samplerCube prefilitered_envmap;
layout(binding = 4) uniform sampler2D brdf_lut;

VS_OUT_PS_IN VS_OUT
{
    vec3 normal;
    vec4 tangent;
    vec3 world_position;
    vec2 tex_coords;
} vs_out;

#section vertex

void vert()
{
    vec4 world_position = vec4(position.xyz, 1.0f) * model_matrix;
    vs_out.world_position = world_position.xyz / world_position.w;
    vs_out.normal = normal * normal_matrix;
    vs_out.tangent = vec4(tangent.xyz * mat3(model_matrix), tangent.w);
    vs_out.tex_coords = uv;

    gl_Position = world_position * view_perspective_matrix;
}

#section fragment

layout(location = 0) out vec4 out_color;
// 生成 [0,1) 的Hammersley点（索引 i，总样本数 N），爱来自DeepSeek
vec2 hammersley2d(uint i, uint N) {
    // 将i的二进制位反转，映射到[0,1)
    vec2 noise = vec2(100, 50) * sin(time);
    float phi = float(bitfieldReverse(i)) * 2.3283064365386963e-10f; // 1/(2^32)
    return fract(noise + vec2(float(i)/float(N), phi));
}

// RTR4 9.41 P340，GGX分布
float D_GGX(float dotNH, float roughness)
{
    // 为了让粗糙度更线性，暴露给用户的roughness是GGX公式中alpha的开方
    float alpha  = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom  = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
// https://learnopengl-cn.github.io/07%20PBR/01%20Theory/
// 计算了微表面上的遮蔽和阴影
// 这里的k是用于拟合准确几何项的经验参数，在不同的情况下取不同的值
// 在计算环境光时 k = alpha / 2，这是Karis 2013在做环境光照时重新拟合的，也可以用老版本 k = (alpha + 1) ** 2 / 8
// 实际上G_1有精确解，见GGX 原始论文，Schlick在这里也做了一个近似，才有Schlick-GGX函数
float G_SchlicksmithGGX_IBL(float dotNL, float dotNV, float roughness)
{
    float alpha  = roughness * roughness;
    float k  = alpha / 2;
    float GL = dotNL / (dotNL * (1.0 - k) + k); // Schlick-GGX 计算遮蔽或者阴影，RTR4上没找到？
    float GV = dotNV / (dotNV * (1.0 - k) + k); 
    return GL * GV; // 使用Smith的方法乘起来计算两个效应的叠加，RTR4上有这个
}

// UE4的GGX-Smith Correlated Joint 近似方案
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
// from https://zhuanlan.zhihu.com/p/81708753
float V_SmithJointApprox(float NoV, float NoL, float roughness)
{
    float a = roughness * roughness;
    float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
    float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
    return 0.5 / (Vis_SmithV + Vis_SmithL);
}

// Fresnel function ----------------------------------------------------
// 使用基础反射率F0和入射角近似计算反射率F
float Pow5(float x)
{
    return (x * x * x * x * x);
}
// 这是一种拟合方式
vec3 F_Schlick(float cosTheta, vec3 F0) 
{ 
    return F0 + (1.0 - F0) * Pow5(1.0 - cosTheta); 
}
// 这是另外一种
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * Pow5(1.0 - cosTheta);
}

// Specular and diffuse BRDF composition --------------------------------------------
vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float roughness, float metallic)
{
    // Precalculate vectors and dot products
    vec3  H     = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0001, 1.0); // 修复法线表面“不可见”处黑色的问题，参考Godot
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    // D 微表面法线分布
    float D = D_GGX(dotNH, max(0.05, roughness));
    // G = Geometric shadowing term (Microfacets shadowing)
    float G = G_SchlicksmithGGX_IBL(dotNL, dotNV, roughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(dotNV, F0);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.00001); // 高光BRDF
    
    // 计算漫反射
    vec3 diff = albedo / PI; // lambert diffuse
    // 漫反射是由折射光在的次表面散射产生的，并且金属没有次表面散射，由此计算系数kD
    vec3 kD = (vec3(1) - F) * (1.0 - metallic);
    return kD * diff + spec;
}

// Split Sum
vec3 SpecularIBL(vec3 F0, float Roughness, vec3 N, vec3 V)
{
    // 这里忽略了dot(N, V) < 0的情况
    vec3 L = 2 * dot(V, N) * N - V; // 反向计算入射光方向
    vec3 env_light = textureLod(prefilitered_envmap, L, Roughness * (textureQueryLevels(prefilitered_envmap) - 1)).rgb;
    vec2 lut = textureLod(brdf_lut, vec2(dot(N, V), Roughness), 0).rg;

    vec3 brdf = env_light * (F0 * lut.r + lut.g);
    return brdf; 
}

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
#ifdef USE_NORMAL_MAP
    // 施密特正交化
    const vec3 tangent = normalize(vs_out.tangent.xyz - normal * dot(normal, vs_out.tangent.xyz));
    // 规定死的副切线计算方法
    const vec3 bitangent = cross(normal, tangent) * vs_out.tangent.w;
 
    vec3 h = texture(normal_texture, vs_out.tex_coords).xyz * 2 - 1;
    vec3 world_normal = normalize(tangent * h.x + bitangent * h.y + normal * h.z);
#else
    vec3 world_normal = normal;
#endif
    return world_normal;
}

void frag()
{
    const vec3 dielectric_specular = vec3(0.04);//一般电解质的基础反射率

    const vec3 N = caculate_normal();//法线
    const vec3 V = normalize(camera_position - vs_out.world_position); // 观察方向
    
#ifdef USE_ORM_TEXTURE
    const vec3 orm = texture(orm_texture, vs_out.tex_coords).xyz; // Occlusion/Roughness/Metallic 
    const float occlusion = orm.x; // 环境光遮蔽（预烘培的）
    const float roughness = roughness_factor * orm.y;//粗糙度
    const float metallic = metallic_factor * orm.z;//金属度
#else
    const float occlusion = 1.0f; // 环境光遮蔽（预烘培的）
    const float roughness = roughness_factor;//粗糙度
    const float metallic = metallic_factor;//金属度
#endif

    const vec3 albedo = texture(basecolor_texture, vs_out.tex_coords).xyz;//基础色
    // 对于金属，其反射率就是albedo。对于一般电介质，其反射率取一般值0.04。不考虑半导体。
    // 反射率F用于计算Fresnel反射
    const vec3 F0 = mix(dielectric_specular, albedo, metallic);

    vec3 result_color = vec3(0);
    
    result_color += SpecularIBL(F0, roughness, N, V) * occlusion; // 高光部分
    result_color += ambient_light * albedo * occlusion; // 低频漫反射部分，暂时用ambient_light替代

    // 点光源
    for(uint i = 0;i < pointlight_num;i++){
        vec3 light_pos = pointlight_list[i].position;
        vec3 light_intensity = pointlight_list[i].intensity;

        // 入射光方向
        vec3 L = normalize(light_pos - vs_out.world_position);

        float squared_distance = dot(light_pos - vs_out.world_position, light_pos - vs_out.world_position);

        vec3 point_light_indensity = pointlight_list[i].intensity / squared_distance;
        // 点光源照到物体表面的强度 * 物体在此方向上的反射率 * 入射光与物体法线夹角的cos
        result_color += point_light_indensity * BRDF(L, V, N, F0, albedo, roughness, metallic) * max(dot(L, N), 0.0f);
        //result_color = light_intensity;
    }

# ifdef GYA_FOG_EXP
    // 指数雾
    const vec3 fog_color = vec3(1.0f ,1.0f ,1.0f);
    float distance = max(0.0f, length(vs_out.world_position - camera_position) - fog_min_distance);
    float fog_factor = exp(-distance * fog_density);
    result_color = mix(fog_color, result_color, fog_factor);
# endif
    
    out_color = vec4(result_color, 1.0f);
    // out_color = vec4(abs(N), 1);
}