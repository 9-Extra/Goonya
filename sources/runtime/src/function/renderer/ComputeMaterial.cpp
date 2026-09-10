#include "ComputeMaterial.h"
#include "function/renderer/PipelineLayout.h"

namespace Goonya {

Ref<ComputeMaterial> ComputeMaterial::clone() const noexcept {
    Ref<ComputeMaterial> c = create_ref<ComputeMaterial>(uber_shader);
    c->local_variant_code = local_variant_code;

    c->parameters = parameters;
    c->textures = textures;
    c->uniform_buffers = uniform_buffers;
    c->shader_storage_buffers = shader_storage_buffers;
    return c;
}

void ComputeMaterial::dispatch_compute(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z) const {
    // 绑定材质的uniform buffer
    if (per_material->get_size() != 0) {
        get_per_material_uniform()->bind_uniform(PER_MATERIAL_UNIFORM_BINDING);
    }
    bind_external_resources();
    get_shader()->dispatch_compute(num_groups_x, num_groups_y, num_groups_z);
}

} // namespace Goonya