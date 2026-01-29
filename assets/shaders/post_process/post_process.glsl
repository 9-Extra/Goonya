#ifndef POST_PROCESS_HEAD
#define POST_PROCESS_HEAD

#ifndef VS_OUT_PS_IN
#if defined(VERTEX_SHADER)
# define VS_OUT_PS_IN out
#elif defined(FRAGMENT_SHADER)
# define VS_OUT_PS_IN in
#endif
#endif

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
#endif