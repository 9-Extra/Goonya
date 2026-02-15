#include "IMeshRenderable.h"
#include "function/renderer/RScene.h"

namespace Goonya {

IMeshRenderable::~IMeshRenderable() {
    if (is_registered()) {
        scene->unregister_mesh(this);
    }
};

/**
 * @brief 标记需要更新，可以重复标记
 * 最终会执行RScene::update_mesh，只会更新一次
 * @see RScene::update_mesh
 * @param bit 脏标记
 */
void IMeshRenderable::mark_dirty(DirtyBit bit) noexcept {
    if (!is_registered()) {
        return;
    }

    dirty_bits |= std::to_underlying(bit);
    scene->enqueue_mesh_update(this);
}
} // namespace Goonya