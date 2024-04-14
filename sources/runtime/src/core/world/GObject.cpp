#include "GObject.h"


namespace Goonya {
void GObject::tick(DirtyFlag parent_flag) {
    if (is_disable())
        return; // 跳过不启用的物体

    dirty_flag.append(parent_flag);

    if (dirty_flag.is_transform_dirty()) {
        if (has_parent()) {
            // 子节点的transform为父节点的transform叠加上自身的transform
            root_model_matrix = get_parent().lock()->root_model_matrix * transform.transform_matrix();
            root_normal_matrix = get_parent().lock()->root_normal_matrix * transform.normal_matrix();
        } else {
            // 对于根节点特殊处理
            root_model_matrix = transform.transform_matrix();
            root_normal_matrix = transform.normal_matrix();
        }
    }
    // 更新组件
    // todo：如果在更新组件时组件增删了components怎么办
    for (auto &c : get_components()) {
        c->tick();
    }

    for (auto &child : get_children()) {
        child->tick(dirty_flag); // 更新子节点
    }

    dirty_flag.clear(); // 清理标记
}
}
