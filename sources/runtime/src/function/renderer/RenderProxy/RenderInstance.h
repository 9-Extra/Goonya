#pragma once

#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include <vector>

namespace Goonya::Graphics{

class RenderInstance{
    Ref<Mesh> mesh;
    std::vector<Ref<Material>> materials;
    std::vector<std::byte> per_object_data; 


};

};