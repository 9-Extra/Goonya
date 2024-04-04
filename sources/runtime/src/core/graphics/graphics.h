#pragma once

#include "renderer/Renderer.h"
#include "renderer/RenderResource.h"

namespace Goonya {
namespace Graphics {

void initialize();

inline void drop(){
    //world.clear();
    renderer.clear();
    resources.clear();
    std::cout << "Exit!\n";
}

inline void render(){
    renderer.render();
}

}
}