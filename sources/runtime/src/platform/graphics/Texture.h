#pragma once

#include "core/intrusive_ptr.h"
#include <cstdint>

#include <tuple>

namespace Goonya {
namespace Graphics {


class Texture: public intrusive_ptr_base<Texture>{
public:
    virtual ~Texture() = default;
    virtual void bind(uint32_t binding) const noexcept=0;
};

class Texture2D: public Texture{
public:
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept = 0; // (weight, height)    
};

class TextureCube: public Texture{};




}
}