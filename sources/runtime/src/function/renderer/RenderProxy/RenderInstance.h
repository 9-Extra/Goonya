#pragma once

#include "platform/graphics/Material.h"
#include "platform/graphics/Mesh.h"
#include <vector>

namespace Goonya::Graphics{

class RenderInstance{
    intrusive_ptr<Mesh> mesh;
    std::vector<intrusive_ptr<Material>> materials;
    std::vector<std::byte> per_object_data; 


};

};