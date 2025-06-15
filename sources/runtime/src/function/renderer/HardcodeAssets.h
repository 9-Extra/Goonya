#pragma once

#include "core/cgmath.h"
#include "platform/graphics/Mesh.h"
#include <cstdint>
#include <vector>

// 硬编码的一些数据
namespace Goonya::Graphics::Assets {

const std::vector<Vector3f> skybox_cube_vertices = {{-1.0, -1.0, -1.0}, {1.0, -1.0, -1.0}, {1.0, 1.0, -1.0},
                                                    {-1.0, 1.0, -1.0},  {-1.0, -1.0, 1.0}, {1.0, -1.0, 1.0},
                                                    {1.0, 1.0, 1.0},    {-1.0, 1.0, 1.0}};

const VertexLayout skybox_cube_vertex_layout{{{VertexAttribute::POSITION, Meta::FieldType::vec3f, 0}},
                                             sizeof(Vector3f)};

const std::vector<uint32_t> skybox_cube_indices = {1, 0, 3, 3, 2, 1, 3, 7, 6, 6, 2, 3, 7, 3, 0, 0, 4, 7,
                                                   2, 6, 5, 5, 1, 2, 4, 5, 6, 6, 7, 4, 5, 4, 0, 0, 1, 5};

struct Vertex {
    Vector3f position;
    Vector3f normal;
    Vector3f tangent;
    Vector2f uv;
};

const std::vector<Vertex> plane_vertices = {
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
};

const VertexLayout plane_vertices_vertex_layout{
    {{VertexAttribute::POSITION, Meta::FieldType::vec3f, offsetof(Vertex, position)},
     {VertexAttribute::NORMAL, Meta::FieldType::vec3f, offsetof(Vertex, normal)},
     {VertexAttribute::TANGENT, Meta::FieldType::vec3f, offsetof(Vertex, tangent)},
     {VertexAttribute::UV, Meta::FieldType::vec2f, offsetof(Vertex, uv)}},
    sizeof(Vertex)};

const std::vector<uint32_t> plane_indices = {0, 1, 2, 2, 3, 0};
} // namespace Goonya::Graphics::Assets