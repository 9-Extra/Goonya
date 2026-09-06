#name skinning
#category compute

#section compute

layout(local_size_x = 128) in;

struct SkinVertex {
    ivec4 joint;
    vec4 weight;
};

struct PMatrix {
    mat4 pose_matrix;
    mat4 normal_matrix;
};

layout(binding = 5, std430) restrict readonly buffer vertex_mesh_buffer{
    float in_vertex[]; // SkinMeshVertex
};
layout(binding = 6, std430) restrict readonly buffer joint_weight{
    SkinVertex skin[];
};

layout(binding = 7, std430) restrict readonly buffer pose_matrix_buffer{
    PMatrix pose_matrix[];
};

layout(binding = 9, std430) restrict writeonly buffer out_vertex_mesh_buffer{
    float out_vertex[]; // SkinMeshVertex
};

// GLSL 无法声明紧凑的SkinMeshVertex结构，改为偏移量访问
/*
struct SkinMeshVertex {
    vec3 position;
    vec3 normal;
    vec4 tangent;
};
*/
const uint MESH_STRIDE = 3 + 3 + 4;

uint get_vertex_count() {
    return in_vertex.length() / MESH_STRIDE;
}

vec3 load_mesh_position(uint x) {
    uint b = MESH_STRIDE * x;
    return vec3(in_vertex[b], in_vertex[b + 1], in_vertex[b + 2]);
}
vec3 load_mesh_normal(uint x) {
    uint b = MESH_STRIDE * x;
    return vec3(in_vertex[b + 3], in_vertex[b + 4], in_vertex[b + 5]);
}
vec4 load_mesh_tangent(uint x) {
    uint b = MESH_STRIDE * x;
    return vec4(in_vertex[b + 6], in_vertex[b + 7], in_vertex[b + 8], in_vertex[b + 9]);
}

void set_mesh_position(uint x, vec3 v) {
    uint b = MESH_STRIDE * x;
    out_vertex[b] = v[0];
    out_vertex[b + 1] = v[1];
    out_vertex[b + 2] = v[2];
}
void set_mesh_normal(uint x, vec3 v) {
    uint b = MESH_STRIDE * x;
    out_vertex[b + 3] = v[0];
    out_vertex[b + 4] = v[1];
    out_vertex[b + 5] = v[2];
}
void set_mesh_tangent(uint x, vec4 v) {
    uint b = MESH_STRIDE * x;
    out_vertex[b + 6] = v[0];
    out_vertex[b + 7] = v[1];
    out_vertex[b + 8] = v[2];
    out_vertex[b + 9] = v[3];
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    if (x >= get_vertex_count()) {
        return;
    }
   
    SkinVertex skin = skin[x];
    vec3 position = load_mesh_position(x);
    vec3 normal = load_mesh_normal(x);
    vec4 tangent = load_mesh_tangent(x);

    mat4 blend_pose_matrix = mat4(0);
    mat4 blend_normal_matrix = mat4(0);
    for (uint i = 0; i < 4; i++){
        if (skin.weight[i] == 0.0) continue;
        PMatrix p = pose_matrix[skin.joint[i]];
        blend_pose_matrix += p.pose_matrix * skin.weight[i];
        blend_normal_matrix += p.normal_matrix * skin.weight[i];
    }
    
    position = (vec4(position, 1.0f) * blend_pose_matrix).xyz;
    normal = normalize((vec4(normal, 0.0f) * blend_normal_matrix).xyz);
    tangent.xyz = normalize((vec4(tangent.xyz, 0.0f) * blend_pose_matrix).xyz);
    // 切线和法线的正交化在像素着色器里做，和插值误差一起消除，这里就不处理了

    set_mesh_position(x, position);
    set_mesh_normal(x, normal);
    set_mesh_tangent(x, tangent);
}
