#name brdf_lut
#category compute

#image output_image access="w" format="rgba16f"
#texture skybox_specular_texture="buildin:black"

#param level=f32(0) // current mipmap level

#section compute

#define SAMPLE_COUNT 1024

#define PI 3.1415926535897932384626433832795

uniform samplerCube skybox_specular_texture;
layout(rgba16f) restrict writeonly uniform imageCube output_image; // only 1 mipmap level

layout(local_size_x = 8, local_size_y = 8, local_size_z = 6) in;

/*
计算用于Split-Sum近似的BRDF预积分天空光照，每个天空盒都需要预计算一份
忽略实际采样中视角方向带来的差异，假定v = n，法线分布lobe以从原点到天空盒上的点的方向为轴
也就是直接对天空盒按照D(n * h, r)进行重要性采样，其中不同r使用mipmap表示

skybox_specular_texture和output_image中的图像实际上是倒置存储的
但用方向采样时skybox_specular_texture是“正”的，写入时需要进行翻转
*/

// 基于mipmap层数线性分配粗糙度，要求至少有不只一层mipmap
// 在采样时逆向映射textureLod(skybox_specular_texture, R, r * (textureQueryLevels(skybox_specular_texture) - 1))
float roughness_inv_mapping(float level) {
    int total_level = textureQueryLevels(skybox_specular_texture);
    if (total_level <= 1) {
        // should never happen!! 
    }
    
    return level / (total_level - 1);
}

// 取(u_1, u_2)
vec2 hammersley2d(uint i, uint N) {
    // 将i的二进制位反转，映射到[0,1)
    float phi = float(bitfieldReverse(i)) * 2.3283064365386963e-10f; // 1/(2^32)
    return fract(vec2(float(i)/float(N), phi));
}

vec3 importance_sample_ggx_h(vec2 u, float r) {
    float alpha = r * r;
    float cos_theta = sqrt((1 - u.x) / (1 + (alpha * alpha - 1) * u.x));
    float phi = 2 * PI * u.y;
    
    float sin_theta = sqrt(1 - cos_theta * cos_theta);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    vec3 h = vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
    return h;
}

vec3 compute_lut(int index, float roughness, vec3 direction) {
    // n = (0, 0, 1)时微表面法线方向
    vec3 h = importance_sample_ggx_h(hammersley2d(index, SAMPLE_COUNT), roughness);
    // 转换到n == direction上
    // 随便选一条和direction垂直的方向
    vec3 temp = abs(direction.z) < 0.99 ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 tangent = cross(direction, temp);
    vec3 bitangent = cross(direction, tangent);

    vec3 world_space_h = h.x * tangent + h.y * bitangent + h.z * direction;
    
    vec3 light = textureLod(skybox_specular_texture, world_space_h, 0).rgb;
    return light;
}

vec3 id_to_direction(uvec3 id) {
    vec2 uv = (vec2(id.xy) + 0.5) / imageSize(output_image).xy;
    vec2 xy = uv * 2 - 1;
    // 0:+X 1:-X 2:+Y 3:-Y 4:+Z 5:-Z
    uint face = id.z;
    vec3 d;
    if (face == 0) d = vec3(1, xy.y, -xy.x);
    if (face == 1) d = vec3(-1, xy.y, xy.x);
    if (face == 2) d = vec3(xy.x, 1, -xy.y);
    if (face == 3) d = vec3(xy.x, -1, xy.y);
    if (face == 4) d = vec3(xy.x, xy.y, 1);
    if (face == 5) d = vec3(-xy.x, xy.y, -1);

    return normalize(d);
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint z = gl_GlobalInvocationID.z; // 6个方向
    if (x >= imageSize(output_image).x || y >= imageSize(output_image).y) {
        return;
    }    

    float roughness = roughness_inv_mapping(level);
    vec3 direction = id_to_direction(gl_GlobalInvocationID);

    vec3 result = vec3(0);
    for(int i = 0; i < SAMPLE_COUNT;i++){
        vec3 value = compute_lut(i, roughness, direction);
        result += value;
    }
    result /= SAMPLE_COUNT;
    // 写入时沿y翻转
    imageStore(output_image, ivec3(x, imageSize(output_image).y - y - 1, z), vec4(result, 1));
}
