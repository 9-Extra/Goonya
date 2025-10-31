#pragma once

#include "craft/core/core.h"
#include "craft/level/LevelRenderer.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vector>

namespace Craft {

class ViewArea {
private:
    uint32_t view_distance = 0;
    uint32_t grid_size = 0;
    Vector3i origin;
    std::vector<ChunkPos> view_area;
public:
    explicit ViewArea(uint32_t view_distance, Vector3i center) : origin(center) {
        set_view_distance(view_distance);
    }
    ~ViewArea() = default;

    void set_view_distance(uint32_t view_distance) {
        if (view_distance == this->view_distance) {
            return;
        }
        this->view_distance = view_distance;
        this->grid_size = view_distance * 2 + 1;
        this->view_area.resize(grid_size * grid_size * grid_size);
    }
    uint32_t get_view_distance() const noexcept {
        return view_distance;
    }

    const Vector3i& get_origin() const noexcept {
        return origin;
    }

    size_t index(Vector3i pos) noexcept {
        Vector3i offset = pos - origin;
        return offset.x * grid_size + offset.z * grid_size + offset.y;
    }
};


}