#include "scene.h"

#include "core/cgmath.h"
#include "core/intrusive_ptr.h"
#include "function/components/CpntCamera.h"
#include "function/components/CpntMeshRender.h"
#include "function/components/CpntPointLight.h"
#include "function/components/CpntSkybox.h"
#include "platform/graphics/Material.h"
#include "resource/Resource.h"
#include "runtime/GoonyaException.h"
#include <fstream>
#include <json/json.h>
#include <memory>
#include <vector>

namespace Goonya::Scene {
// 从json加载一个Vector3f
Vector3f load_vec3(const Json::Value &json) {
    assert(json.isArray());
    return {json[0].asFloat(), json[1].asFloat(), json[2].asFloat()};
}
// 从json加载transform，对不完整或不存在的取默认值
Transform load_transform(const Json::Value &json) {
    Transform trans;
    if (json.isMember("transform")) {
        const Json::Value &t = json["transform"];
        if (t.isMember("position")) {
            trans.position = load_vec3(t["position"]);
        }
        if (t.isMember("rotation")) {
            trans.rotation = Quaternion::from_eular(load_vec3(t["rotation"]));
        }
        if (t.isMember("scale")) {
            trans.scale = load_vec3(t["scale"]);
        }
    }

    return trans;
}

BoundingBox load_bbox(const Json::Value &json) { return BoundingBox{load_vec3(json["min"]), load_vec3(json["max"])}; }

// 从json加载组件
void load_conponents_from_json(GObject *obj, const Json::Value &json) {
    for (const auto &cpnt_name : json.getMemberNames()) {
        const Json::Value &cpnt_desc = json[cpnt_name];
        if (cpnt_name == "mesh_render") {
            std::unique_ptr<Graphics::CpntMeshRender> cpnt_ptr = std::make_unique<Graphics::CpntMeshRender>();
            if (cpnt_desc.isMember("mesh")) {
                cpnt_ptr->set_mesh(Resource::resources.meshes.get(cpnt_desc["mesh"].asString()));
            }
            if (cpnt_desc.isMember("material")) {
                std::vector<intrusive_ptr<Graphics::Material>> materials;
                for (const Json::Value &material_name : cpnt_desc["material"]) {
                    materials.emplace_back(Resource::resources.materials.get(material_name.asString()));
                }
                cpnt_ptr->set_materials(std::span(materials));
            }
            obj->add_component(std::move(cpnt_ptr));
        } else if (cpnt_name == "point_light") {
            Vector3f color = load_vec3(cpnt_desc["color"]);
            float radius = cpnt_desc["factor"].asFloat();
            obj->add_component(std::make_unique<Graphics::CpntPointLight>(color, radius));
        } else if (cpnt_name == "camera") {
            bool is_main = cpnt_desc.isMember("is_main") && cpnt_desc["is_main"].asBool();
            float near_z = cpnt_desc["near_z"].asFloat();
            float far_z = cpnt_desc["far_z"].asFloat();
            float fov = cpnt_desc["fov"].asFloat();
            std::unique_ptr<Graphics::CpntCamera> camera =
                std::make_unique<Graphics::CpntCamera>(is_main, near_z, far_z, fov);
            obj->add_component(std::move(camera));
        } else if (cpnt_name == "sky_box") {
            intrusive_ptr<Graphics::Material> material =
                Resource::resources.materials.get(cpnt_desc["material"].asString());
            bool ignore_range = !(cpnt_desc.isMember("ignore_range") && !cpnt_desc["ignore_range"].asBool());
            BoundingBox bbox;
            if (cpnt_desc.isMember("bbox")) {
                bbox = load_bbox(cpnt_desc["bbox"]);
            } else if (!ignore_range) {
                throw RuntimeError("带范围的天空盒必须指定包围盒");
            }
            obj->add_component(std::make_unique<Graphics::CpntSkybox>(material, ignore_range, bbox));
        } else {
            throw RuntimeError(std::format("未知组件：{}", cpnt_name));
        }
    }
}

// 从json递归加载节点
void load_node_from_json(const Json::Value &node, GObject *root) {
    for (const Json::Value &object_desc : node) {
        const std::string &name = object_desc.isMember("name") ? object_desc["name"].asString() : "";
        auto gobject = std::make_shared<GObject>(load_transform(object_desc), name);
        if (object_desc.isMember("components")) {
            load_conponents_from_json(gobject.get(), object_desc["components"]);
        }

        root->attach_child(gobject);
        if (object_desc.isMember("children")) {
            load_node_from_json(object_desc["children"], root->get_children().back().get());
        }
    }
}

// 从json文件加载场景
Scene load_scene_from_json(const std::string &path) {
    Scene scene;
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        reader.parse(file, json, false);
    }
    // 加载物体
    if (json.isMember("root")) {
        load_node_from_json(json["root"], scene.root.get());
    }

    return scene;
}

} // namespace Goonya::Scene
