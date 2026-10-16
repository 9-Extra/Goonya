#include "ComputeMaterial.h"
#include "function/renderer/PipelineLayout.h"

#include <array>
#include <cstdint>

namespace Goonya {

Ref<ComputeMaterial> ComputeMaterial::clone() const noexcept {
    Ref<ComputeMaterial> c = create_ref<ComputeMaterial>(uber_shader);
    c->local_variant_code = local_variant_code;

    c->parameters = parameters;
    c->textures = textures;
    c->images = images;
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

void ComputeMaterial::dispatch_extent(uint32_t work_x, uint32_t work_y, uint32_t work_z) const {
    // 计算着色器需要保证work_group_size不为0
    const auto [x, y, z] = get_shader()->get_work_group_size();
    // 向上取整，work为0时组数也为0
    dispatch_compute((work_x + x - 1) / x, (work_y + y - 1) / y, (work_z + z - 1) / z);
}

} // namespace Goonya