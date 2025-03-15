#pragma once

#include "core/intrusive_ptr.h"
#include <cstdint>

namespace Goonya{
namespace Graphics {

enum class Topology{
    POINT,
    LINE,
    TRIANGLE
};

struct SubMesh{
    uint32_t start_index;
    uint32_t index_count;
    Topology topology;
};

class Mesh: public intrusive_ptr_base<Mesh>{
public:
    virtual uint32_t get_submesh_count() const noexcept = 0;
    virtual ~Mesh() = default;
};

}    
}