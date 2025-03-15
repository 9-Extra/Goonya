#include "GObject.h"


namespace Goonya {
void GObject::tick(DirtyFlag parent_flag) {
    if (is_disabled())
        return; // 跳过不启用的物体

    dirty_flag.append(parent_flag);

    if (dirty_flag[DirtyFlag::TRANSFORM_DIRTY]) {
        if (has_parent()) {
            // 子节点的transform为父节点的transform叠加上自身的transform
            world_model_matrix = get_parent().lock()->world_model_matrix * transform.model_matrix();
            world_normal_matrix = get_parent().lock()->world_normal_matrix * transform.normal_matrix();
        } else {
            // 对于根节点特殊处理
            world_model_matrix = transform.model_matrix();
            world_normal_matrix = transform.normal_matrix();
        }
    }
    // 更新组件
    // todo：如果在更新组件时组件增删了components怎么办
    for (auto &c : get_components()) {
        c->on_tick();
    }

    for (auto &child : get_children()) {
        child->tick(dirty_flag); // 更新子节点
    }

    dirty_flag.clear(); // 清理标记
}
}
