#pragma once

#include "core.h"
#include "core/cgmath.h"

#include <array>
#include <cstdint>
#include <ranges>
#include <vector>

namespace Craft{

/**
* @brief 一个立方体模型，作为完整方块模型的组成部分
* 未烘培，可以直接从文件读取得到
*/
struct BlockElement{
    struct Face{
        Direction direction;
        Goonya::Vector2f uv_up_left;
        Goonya::Vector2f uv_down_right;
        std::string texture_key; 
    };

    Goonya::Vector3f from; // 西下南角（最小）
    Goonya::Vector3f to; // 东上北角（最大）
    Goonya::Quaternion rotation = Goonya::Quaternion::identity();
    std::vector<Face> faces;
};

struct BlockModel{
    std::vector<BlockElement> elements;
};

struct BlockVertex{ // NOLINT
    Goonya::Vector3f position;
    Goonya::Vector2f uv;
};

// 一个四边形渲染数据，尽可能预计算需要传入GPU的顶点数据（不是所有）
struct BakedQuad{ // NOLINT
    std::array<BlockVertex, 4> vertices;
    Direction normal;
    uint32_t color_texture_index = 0; // 颜色纹理在对应纹理数组中的下标
};

struct BakedBlockModel{
    std::array<std::vector<BakedQuad>, 6> culled_quads; // 在6个方向上可以裁剪的表面，如果此方向可裁剪，则可以忽略对应的std::vector<BakedQuad> 
    std::vector<BakedQuad> unculled_quads; // 不能裁剪的其他表面，不可以忽略

    void merge(const BakedBlockModel& another) noexcept {
        for(const auto& [this_quads, another_quads]: std::views::zip(culled_quads, another.culled_quads)){
            this_quads.insert(this_quads.end(), another_quads.begin(), another_quads.end());           
        }
        unculled_quads.insert(unculled_quads.end(), another.unculled_quads.begin(), another.unculled_quads.end());
    }
};

};