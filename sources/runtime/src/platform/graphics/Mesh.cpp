#include "Mesh.h"
#include "platform/graphics/Graphics.h"

namespace Goonya::Graphics {

Ref<Mesh> MeshContainer::load(const MeshDesc &desc) const {
    Ref<Mesh> mesh = graphics_api->create_mesh();
    mesh->set_layout(desc.vertex_layout);

    Ref<Buffer> vertex_buffer =
        graphics_api->create_buffer((uint32_t)desc.raw_vertices.size(), BufferType::STATIC);
    vertex_buffer->write(std::span(desc.raw_vertices), 0);
    mesh->set_vertex_buffer(vertex_buffer);

    Ref<Buffer> indices_buffer =
        graphics_api->create_buffer(uint32_t(desc.indices.size() * sizeof(uint32_t)), BufferType::STATIC);
    indices_buffer->write(std::span((std::byte *)desc.indices.data(), desc.indices.size() * sizeof(uint32_t)), 0);
    mesh->set_indices_buffer(indices_buffer);

    mesh->submeshes = desc.sub_meshes;

    return mesh;
}
} // namespace Goonya::Graphics