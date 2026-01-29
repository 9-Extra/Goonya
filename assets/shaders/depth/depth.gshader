#section meta

name depth

#section common
#include "shaders/common.glsl"

#section vertex
void vert() {
    gl_Position = vec4(position.xyz, 1.0f) * model_matrix * view_perspective_matrix;
}

#section fragment
void frag() {
    // noop
}