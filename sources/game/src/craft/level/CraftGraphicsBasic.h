#pragma once

#include "core/cgmath.h"
#include "platform/graphics/Mesh.h"
#include <cstdint>

namespace Craft {

struct TerrainMeshVertex {
    Goonya::Vector3f position;
    Goonya::Vector2f uv;
};

struct alignas(16) TerrainPerSurface { // NOLINT
    alignas(16) uint32_t basecolor_id;
    alignas(16) Goonya::Vector3f normal;
};

struct PointLight {
    alignas(16) Goonya::Vector3f position;
    alignas(16) Goonya::Vector3f intensity;
};

// struct TerrainPerFrame // NOLINT
// {
//     Goonya::Matrix4 view_perspective_matrix; //16 * 4
//     alignas(16) Goonya::Vector3f ambient_light; //3 * 4 + 4
//     alignas(16) Goonya::Vector3f camera_position;
//     alignas(4) float fog_min_distance;
//     alignas(4) float fog_density;
//     alignas(4) float time;
//     alignas(4) uint pointlight_num; // 4
//     PointLight pointlight_list[2];
// };

inline const Goonya::Graphics::VertexLayout VERTEX_LAYOUT_PLANE =
    Goonya::Graphics::VertexLayoutBuilder()
        .add_attribute(Goonya::Graphics::VertexAttribute::POSITION)
        .add_attribute(Goonya::Graphics::VertexAttribute::UV)
        .build();

constexpr std::string_view TERRAIN_SHADER_NAME = "shaders/craft/terrain/terrain";

} // namespace Craft