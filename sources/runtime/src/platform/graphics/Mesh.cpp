#include "Mesh.h"
#include "platform/graphics/Graphics.h"

namespace Goonya::Graphics {

Ref<Mesh> MeshContainer::load(const MeshDesc &desc) const {
    Ref<Mesh> mesh = graphics_api->create_mesh(desc.vertex_layout);

    mesh->set_vertices(0, desc.raw_vertices);
    mesh->set_indices(desc.indices);

    mesh->submeshes = desc.sub_meshes;

    return mesh;
}
} // namespace Goonya::Graphics