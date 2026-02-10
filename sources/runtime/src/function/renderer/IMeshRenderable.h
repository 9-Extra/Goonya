#pragma once

#include "core/cgmath/transform.h"
#include "core/sparse_set.h"
#include "function/renderer/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include <cstddef>

namespace Goonya {

struct Instance;
class RScene;

class IMeshRenderable {
private:
    friend class RScene;
    RScene *scene = nullptr;
    size_t index_in_scene = InvalidIndex;
    size_t pending_update_index = InvalidIndex; // 在更新队列中的索引, InvalidIndex表示不在其中

    Ref<GLBuffer> per_object_uniform;
    std::vector<Handle<Instance>> instance_indices;
    uint8_t dirty_bits = std::to_underlying(DirtyBit::None);

public:
    constexpr static size_t InvalidIndex = std::numeric_limits<size_t>::max();
    enum class DirtyBit : uint8_t {
        None = 0,
        Transform = 1 << 1,
        Material = 1 << 2,
        Mesh = 1 << 3,

        Init = Transform | Material | Mesh,
    };

    bool hidden : 1 = false;
    Ref<GLMesh> mesh;
    std::vector<Ref<Material>> materials;
    Transform transform;

    IMeshRenderable() = default;
    virtual ~IMeshRenderable();

    RScene *get_scene() const { return scene; }

    bool is_hidden() const { return hidden; }
    void hide(bool hide = true) {
        hidden = hide;
        mark_dirty(DirtyBit::Mesh);
    }
    bool is_registered() const { return scene != nullptr; }

    void mark_dirty(DirtyBit bit) noexcept;
};

} // namespace Goonya