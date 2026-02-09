#include "IMeshRenderable.h"
#include "function/renderer/RScene.h"

namespace Goonya {

IMeshRenderable::~IMeshRenderable() {
    if (is_registered()) {
        scene->unregister_mesh(this);
    }
};

void IMeshRenderable::mark_dirty(DirtyBit bit) noexcept {
    if (!is_registered()) {
        return;
    }

    dirty_bits |= std::to_underlying(bit);
    scene->enqueue_mesh_update(this);
}
} // namespace Goonya