#name pbr
#texture basecolor_texture="buildin:missing_texture"
#texture normal_texture="buildin:normal"
#texture orm_texture="buildin:white"
#texture skybox_specular_texture="buildin:black"

#param metallic_factor=f32(1)
#param roughness_factor=f32(1)

#global_variant GYA_IBL_ENVIRONMENT_LIGHT
#global_variant GYA_FOG_EXP

#local_variant _ USE_NORMAL_MAP
#local_variant _ USE_ORM_TEXTURE

#section common
#include "shaders/common.glsl"

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D orm_texture;
layout(binding = 5) uniform samplerCube skybox_specular_texture;

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
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float alpha  = roughness * roughness;
    float k  = alpha / 2;
    float GL = dotNL / (    dotNL * (1.0 - k) + k); // Schlick-GGX 计算遮蔽或者阴影，RTR4上没找到？
    float GV = dotNV / (dotNV * (1.0 - k) + k); 
    return GL * GV; // 使用Smith的方法乘起来计算两个效应的叠加，RTR4上有这个
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
    float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
    // F = Fresnel factor (Reflectance depending on angle of incidence)
    vec3 F = F_Schlick(dotNV, F0);

    vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.00001); // 高光BRDF
    
    // 计算漫反射
    vec3 diff = albedo / PI; // lambert diffuse
    // 漫反射是由折射光在的次表面散射产生的，并且金属没有次表面散射，由此计算系数kD
    vec3 kD = (vec3(1) - F) * (1.0 - metallic);
    return kD * diff + spec;
}

// 爱来自虚幻4，https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// 输入二维(0, 1)随机数Xi（可用低差异序列），粗糙度Roughness和法线方向N
// 使用D(H)H*N作为提议分布，采样服从此分布一个球极坐标，最后转换为世界空间方向向量
vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    vec3 H; // 位于切线空间的方向，法线方向为+z
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    // 使用法线方向N随便构造一组基以转换到世界坐标系，即随便找两个和N垂直的方向向量（法线分布各向同性，与切线方向无关）
    // 理论上随便找个向量和N叉乘就能找到与N垂直的向量，为了数值精度这个向量不能和N太相近
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize(cross(UpVector, N)); // 找一个和N垂直的方向
    vec3 TangentY = cross(N, TangentX);
    // Tangent to world space
    //return normalize(vec3(1, 1, 1));
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

// 爱来自虚幻4，https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec3 SpecularIBL(vec3 SpecularColor, float Roughness, vec3 N, vec3 V)
{
    vec3 SpecularLighting = vec3(0);
    const uint NumSamples = 32;
    float NoV = max(dot(N, V), 0);
    for(uint i = 0; i < NumSamples; i++)
    {
        vec2 Xi = hammersley2d(i, NumSamples);
        vec3 H = ImportanceSampleGGX(Xi, Roughness, N); // 采样得到世界空间半角向量
        vec3 L = 2 * dot(V, H) * H - V; // 反向计算入射光方向
        float NoL = max(dot(N, L), 0);
        float NoH = max(dot(N, H), 0);
        float VoH = max(dot(V, H), 0);
        
        if (NoL > 0)
        {
            vec3 light = textureLod(skybox_specular_texture, L, 0).xyz;
            float G = G_SchlicksmithGGX(NoL, NoV, Roughness);
            vec3 F = F_Schlick(VoH, SpecularColor); // VoH == LoH
            
            /* 
            本来应该是 输出亮度 = 入射光light * 高光brdf / 概率密度pdf
            其中 pdf = D * NoH / (4 * VoH) 有D = D_GGX(NoH, Roughness)
            入射光 light = SampleColor * NoL
            高光brdf = D * G * F / (4 * NoL * NoV)
            
            化简后得到下面实际计算用的式子，GGX在重要性采样中被约去不用计算
            */
            SpecularLighting += light * F * G * VoH / (NoH + 0.000001);
        }
    }
    return SpecularLighting / (NoV + 0.000001) / NumSamples;
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

    // 环境光
#ifdef GYA_IBL_ENVIRONMENT_LIGHT
    // 使用环境光贴图
    result_color += SpecularIBL(F0, roughness, N, V) * occlusion;
    result_color += ambient_light * albedo * occlusion; // 低频漫反射部分，暂时用ambient_light替代
#else
    // 使用常数环境光强度
    result_color += ambient_light * albedo * occlusion;
#endif

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