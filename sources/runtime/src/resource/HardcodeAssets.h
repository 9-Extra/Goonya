#pragma once

#include "core/cgmath/aabb.h"
#include "core/cgmath/cgmath.h"
#include "platform/graphics/opengl/GLMesh.h"
#include <cstdint>
#include <vector>

// 硬编码的一些数据
namespace Goonya::Assets {

inline const std::vector<Vector3f> skybox_cube_vertices = {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0},
                                                           {-1.0, 1.0, -1.0},  {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0},
                                                           {1.0, 1.0, 1.0},    {-1.0, 1.0, 1.0}};

inline const BoundingBox skybox_cube_aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};

inline const VertexLayout skybox_cube_vertex_layout =
    VertexLayoutBuilder().add_attribute(VertexAttribute::POSITION).build();

inline const std::vector<uint32_t> skybox_cube_indices = {1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
                                                          2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector4f tangent;
    Vector2f uv;
};

inline const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
};

inline const BoundingBox plane_aabb = {{-1.0f, -1.0f, -0.0001f}, {1.0f, 1.0f, 0.0001f}};

inline const VertexLayout plane_vertices_vertex_layout = VertexLayoutBuilder()
                                                             .add_attribute(VertexAttribute::POSITION)
                                                             .add_attribute(VertexAttribute::NORMAL)
                                                             .add_attribute(VertexAttribute::TANGENT)
                                                             .add_attribute(VertexAttribute::UV)
                                                             .build();

inline const std::vector<uint32_t> plane_indices = {0, 1, 2, 2, 3, 0};
} // namespace Goonya::Assets
