#pragma once

#include "core/intrusive_ptr.h"

namespace Goonya{
namespace Graphics {

enum class Topology{
    POINT,
    LINE,
    TRIANGLE
};

class Mesh: public intrusive_ptr_base<Mesh>{
public:
    virtual void bind() const = 0;

    virtual uint32_t get_indices_count() = 0;
    
    virtual ~Mesh() = default;
};

}    
}