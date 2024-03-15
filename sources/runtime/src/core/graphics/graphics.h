#pragma once

#include "graphics_device.h"
namespace Goonya {
namespace Graphics {

inline void initialize(){
    Detail::devices.init();
}

inline void drop(){
    Detail::devices.drop();
}

inline void swap(){
    Graphics::Detail::devices.p_swap_chain->Present(1, 0);
}

}
}