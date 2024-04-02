#pragma once

#include "renderer/Renderer.h"
#include "renderer/RenderResource.h"
#include "core/display/display.h"

namespace Goonya {
namespace Graphics {

void initialize();

inline void drop(){
    //world.clear();
    renderer.clear();
    resources.clear();
    std::cout << "Exit!\n";
}

inline void swap(){
    renderer.render();
    Display::swap();
}

}
}