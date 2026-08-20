# GShader 编程指南

`gshader` 是 Goonya 引擎自定义的元着色器（UberShader）格式。一个 `.gshader` 文件会被引擎解析并编译为一对 OpenGL GLSL 着色器（顶点 + 片段），支持预处理指令、自动材质参数块生成、着色器变体等特性。

---

## 1. 基本结构

一个典型的 `gshader` 文件由**预处理指令**和**代码段**组成：

```gshader
#name my_shader                // 着色器名称
#texture albedo="buildin:white" // 声明默认纹理
#param tint=vec3(1, 1, 1)      // 声明材质参数
#setting cull_mode="back"      // 设置管线状态

#section common
#include "shaders/common.glsl"

VS_OUT_PS_IN VS_OUT
{
    vec2 uv;
} vs_out;

#section vertex

void vert()
{
    vs_out.uv = uv;
    gl_Position = vec4(position, 1.0) * model_matrix * view_perspective_matrix;
}

#section fragment

layout(binding = 0) uniform sampler2D albedo;
layout(location = 0) out vec4 out_color;

void frag()
{
    out_color = vec4(texture(albedo, vs_out.uv).rgb * tint, 1.0);
}
```

**关键约定：**
- 你需要在 `vertex` 段定义 `void vert()`，在 `fragment` 段定义 `void frag()`。
- 引擎会自动为你生成 `void main() { vert(); }` / `void main() { frag(); }`。
- 最终编译的目标版本为 **OpenGL 4.4 Core**。

---

## 2. 预处理指令

所有指令均以 `#` 开头，每行一条。

### 2.1 `#name <name>`

声明着色器的显示名称。

```gshader
#name pbr
```

### 2.2 `#texture <name>="<resource>"`

声明本着色器使用的纹理，并指定默认资源。

```gshader
#texture basecolor_texture="buildin:missing_texture"
#texture normal_texture="buildin:normal"
```

- 纹理会按照**声明顺序**依次分配到纹理单元 `0, 1, 2, ...`。
- 在 GLSL 中需要手动声明对应的 `sampler2D` / `samplerCube` / `sampler2DArray` 等，**名称必须与这里声明的一致**。
- 若 `#texture` 声明了某纹理却在 GLSL 中没有使用，或 GLSL 中使用了未在 `#texture` 中声明的纹理，均会报错。

### 2.3 `#param <name>=<type>(<value>)`

声明材质参数（Material Parameter）。引擎会自动将这些参数收集到一个 `per_material` Uniform Block 中，并在编译时插入到 `common` 段的开头。

```gshader
#param metallic_factor=f32(1)
#param color_tint=vec3(1.0, 0.5, 0.2)
#param use_fog=bool(true)
```

**支持的类型与 GLSL 映射：**

| gshader 类型 | GLSL 类型 | 示例 |
|---|---|---|
| `bool` | `bool` | `bool(true)` |
| `int` | `int` | `int(42)` |
| `f32` | `float` | `f32(0.5)` |
| `vec2` | `vec2` | `vec2(1.0, 0.0)` |
| `vec3` | `vec3` | `vec3(1, 1, 1)` |
| `vec4` | `vec4` | `vec4(1, 0, 0, 1)` |
| `mat4` | `mat4` | `mat4(1,0,0,0, 0,1,0,0, ...)` |

> **注意**：空的括号表示取该类型的默认值（如 `vec3()` 等价于 `vec3(0,0,0)`）。

### 2.4 `#setting <key>="<value>"`

设置该着色器默认的渲染管线（Pipeline）状态。

| 键 | 可选值 | 说明 |
|---|---|---|
| `depth_test` | `off`, `less`, `less_equal`, `greater`, `greater_equal`, `equal`, `not_equal`, `never`, `always` | 深度测试模式 |
| `cull_mode` | `off`, `front`, `back` | 面剔除模式 |
| `write_mask` | `r`, `g`, `b`, `a`, `d`, `s`, `0` 的组合字符串 | 颜色/深度/模板写入掩码。例如 `d` 表示只写深度，`0` 表示全部不写，`rgba` 表示只写颜色 |
| `render_priority` | 整数 | 渲染优先级，数值小的先绘制。`OPAQUE` 默认为 `1000` |

```gshader
#setting depth_test="less_equal"
#setting cull_mode="off"
#setting write_mask="d"
#setting render_priority="1001"
```

### 2.5 `#global_variant <key>`

声明**全局着色器变体**（Global Variant）。全局变体一旦启用，会影响**所有**使用该着色器的材质。

```gshader
#global_variant GYA_IBL_ENVIRONMENT_LIGHT
#global_variant GYA_FOG_EXP
```

在代码中配合 `#ifdef` 使用：

```glsl
#ifdef GYA_IBL_ENVIRONMENT_LIGHT
    result_color += SpecularIBL(...);
#else
    result_color += ambient_light * albedo;
#endif
```

全局变体由引擎全局状态管理，单个着色器仅声明自己关心哪些全局关键字。

### 2.6 `#local_variant <key1> <key2> ...`

声明**局部着色器变体**（Local Variant）。局部变体按**组**定义，**同一组内同一时刻只能启用一个**。

```gshader
#local_variant _ USE_NORMAL_MAP
#local_variant _ USE_ORM_TEXTURE
```

- `_`（下划线）表示该组的默认状态：**不定义任何宏**。
- 上例中，第一组可选 `无` 或 `USE_NORMAL_MAP`，第二组可选 `无` 或 `USE_ORM_TEXTURE`。

也可以在多选一的场景下省略 `_`：

```gshader
#local_variant HORIZONTAL VERTICAL
```

局部变体针对**单个材质**设置，不会影响其他材质。

> **注意**：同一着色器中，全局变体关键字与局部变体关键字**不可重复**。

### 2.7 `#section <name>`

标记后续代码所属的段。

| 段名 | 说明 |
|---|---|
| `common` | 公共代码，会被同时插入到顶点着色器和片段着色器中 |
| `vertex` | 顶点着色器代码，需包含 `void vert()` |
| `fragment` | 片段着色器代码，需包含 `void frag()` |
| `end` | （可选）显式结束当前段，回到无段状态 |

```gshader
#section common
// 这里放 VS 和 PS 共用的代码

#section vertex
void vert() { ... }

#section fragment
void frag() { ... }
```

### 2.8 `#include "<path>"`

包含外部 GLSL 文件。只能在**段内**使用。

```gshader
#section common
#include "shaders/common.glsl"
```

- 搜索路径：先尝试在**当前 `.gshader` 所在目录**查找，再在**项目根目录**查找。
- 目前**暂不支持**被包含文件内部再出现 `#include`（即单层包含）。

---

## 3. 代码编写规范

### 3.1 顶点与片段间的数据传递

使用 `VS_OUT_PS_IN` 宏 + `VS_OUT` 接口块（Interface Block）来传递数据：

```glsl
VS_OUT_PS_IN VS_OUT
{
    vec3 world_position;
    vec2 tex_coords;
    flat uint object_id;  // 可以使用 flat, noperspective 等修饰符
} vs_out;
```

- `VS_OUT_PS_IN` 在顶点着色器中被定义为 `out`，在片段着色器中被定义为 `in`。
- 访问时在 VS 中写入 `vs_out.tex_coords = uv;`，在 PS 中读取 `vs_out.tex_coords`。

### 3.2 顶点输入属性

引擎固定了以下顶点属性布局（在 `common.glsl` 或引擎内部已声明）：

| Location | 名称 | 类型 |
|---|---|---|
| 0 | `position` | `vec3` |
| 1 | `normal` | `vec3` |
| 2 | `tangent` | `vec4` |
| 3 | `color` | `vec3` |
| 4 | `uv` | `vec2` |

通常不需要自己再写 `layout(location = N) in ...`，直接 `#include "shaders/common.glsl"` 即可。

### 3.3 内置 Uniform Block

同样由 `common.glsl` 提供：

**`per_frame`（binding = 0）**
```glsl
layout(binding = 0, std140) uniform per_frame
{
    mat4 view_matrix;
    mat4 view_matrix_inv;
    mat4 perspective_matrix;
    mat4 view_perspective_matrix;
    vec3 ambient_light;
    vec3 camera_position;
    float fog_min_distance;
    float fog_density;
    vec2 screen_size;
    uint pointlight_num;
    PointLight pointlight_list[POINTLIGNT_MAX]; // POINTLIGNT_MAX = 8
};
```

**`per_object`（binding = 1）**
```glsl
layout(binding = 1) uniform per_object
{
    mat4 model_matrix;
    mat3 normal_matrix;
};
```

**`per_material`**
由引擎根据 `#param` 指令**自动生成**，绑定到一个固定 binding 点。开发者**无需手动书写**，只需在代码中直接使用参数名。

### 3.4 变体与条件编译

变体通过宏注入实现。引擎会在编译时把对应的关键字以 `#define KEY` 的形式插入到 `#pragma GYA_INJECT` 的位置。

因此你只需在 GLSL 中使用标准预处理：

```glsl
#ifdef USE_NORMAL_MAP
    // ...
#elif defined(USE_PARALLAX_MAP)
    // ...
#else
    // ...
#endif
```

### 3.5 入口函数

- 顶点着色器入口：**`void vert()`**
- 片段着色器入口：**`void frag()`**

不要自己写 `void main()`，引擎会自动包装。

### 3.6 输出

- 顶点着色器：至少写入 `gl_Position`。
- 片段着色器：使用 `layout(location = 0) out vec4 out_color;` 输出颜色。若需多目标渲染，自行添加更多 `layout(location = N) out ...`。

---

## 4. 编译流程简述

1. **解析**：读取 `.gshader`，按指令提取名称、纹理、参数、变体、管线状态、各段代码。
2. **生成 `per_material` 块**：根据 `#param` 生成 `uniform per_material { ... }`，追加到 `common` 段开头。
3. **组装 VS / PS**：
   - VS = `#version 440 core` + `#define VERTEX_SHADER` + `#pragma GYA_INJECT` + `common` + `vertex` + `void main() { vert(); }`
   - PS = `#version 440 core` + `#define FRAGMENT_SHADER` + `#pragma GYA_INJECT` + `common` + `fragment` + `void main() { frag(); }`
4. **变体编译**：当材质请求某个变体时，引擎把该变体对应的所有宏定义替换掉 `#pragma GYA_INJECT`，再编译成真正的 `GLShader`。

---

## 5. 完整示例

### 5.1 PBR 着色器（`assets/shaders/pbr/pbr.gshader`）

```gshader
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

layout(binding = 0) uniform sampler2D basecolor_texture;
layout(binding = 1) uniform sampler2D normal_texture;
layout(binding = 2) uniform sampler2D orm_texture;
layout(binding = 5) uniform samplerCube skybox_specular_texture;

layout(location = 0) out vec4 out_color;

vec3 caculate_normal(){
    const vec3 normal = normalize(vs_out.normal);
#ifdef USE_NORMAL_MAP
    const vec3 tangent = normalize(vs_out.tangent.xyz - normal * dot(normal, vs_out.tangent.xyz));
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
    // ... PBR 计算 ...
    out_color = vec4(result_color, 1.0f);
}
```

### 5.2 后处理着色器（`assets/shaders/post_process/basic.gshader`）

```gshader
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

uniform sampler2D source;
uniform sampler2D bloom;

layout(location = 0) out vec4 out_color;

void frag()
{
    vec3 pixel = textureLod(source, vs_out.uv, 0).rgb;
#if defined(BLOOM)
    vec3 bloom_pixel = textureLod(bloom, vs_out.uv, 0).rgb;
    pixel = 1 - (1 - pixel) * (1 - bloom_pixel);
#endif
    out_color = vec4(pow(pixel, vec3(1 / 2.2)), 1.0f);
}
```

---

## 6. 常见问题

**Q1：为什么我不能在 `#section` 外面写 `#version`？**
> 引擎已经自动注入 `#version 440 core`。若写了自定义的 `#version`，解析器会将其视为未知指令并报错（除非它位于某段内部）。

**Q2：为什么纹理声明了却在运行时报错？**
> 检查两点：
> 1. `#texture` 中的名称与 GLSL 中的采样器变量名必须完全一致。
> 2. 如果使用了 `layout(binding = N)`，确保 `N` 与 `#texture` 的声明顺序对应（第一个 `#texture` 对应 binding 0，以此类推）。

**Q3：局部变体的 `_` 是什么意思？**
> 表示该组的默认分支：**不定义任何宏**。例如 `#local_variant _ USE_NORMAL_MAP` 表示这一组有两种可能：普通状态（无宏）或启用法线贴图（定义 `USE_NORMAL_MAP`）。

**Q4：如何调试生成的 GLSL？**
> 目前引擎没有直接输出生成后 GLSL 的接口。如果着色器编译失败，通常会在日志中输出错误信息和对应的源码片段。可以通过断点或临时日志在 `GLShader.cpp` / `UberShader.cpp` 中查看最终生成的字符串。

---

*本文档基于 Goonya 引擎当前实现编写，如有引擎更新导致行为变化，请以代码为准。*
