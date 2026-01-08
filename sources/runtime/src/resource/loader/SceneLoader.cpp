#include "SceneLoader.h"

#include "core/RefCount.h"
#include "core/cgmath/cgmath.h"
#include "function/components/CpntCamera.h"
#include "function/components/CpntMeshRender.h"
#include "function/components/CpntPointLight.h"
#include "function/components/CpntSkybox.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/Material.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "resource/ResMng.h"
#include "runtime/GoonyaException.h"
#include "core/path_formatter.h"

#include <fstream>
#include <json/json.h>
#include <memory>
#include <vector>

namespace Goonya {
// 从json加载一个Vector3f
Vector3f load_vec3(const Json::Value &json) {
    GN_ASSERT(json.isArray());
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
            std::unique_ptr<CpntMeshRender> cpnt_ptr = std::make_unique<CpntMeshRender>();
            if (cpnt_desc.isMember("mesh")) {
                cpnt_ptr->set_mesh(resources.load_resource<GLMesh>(cpnt_desc["mesh"].asString()));
            }
            if (cpnt_desc.isMember("material")) {
                std::vector<Ref<Material>> materials;
                for (const Json::Value &material_name : cpnt_desc["material"]) {
                    std::string mat_name = material_name.asString();
                    materials.emplace_back(mat_name.empty() ? nullptr : resources.load_resource<Material>(mat_name));
                }
                cpnt_ptr->set_materials(std::span(materials));
            }
            obj->add_component(std::move(cpnt_ptr));
        } else if (cpnt_name == "point_light") {
            Vector3f color = load_vec3(cpnt_desc["color"]);
            float radius = cpnt_desc["factor"].asFloat();
            obj->add_component(std::make_unique<CpntPointLight>(color, radius));
        } else if (cpnt_name == "camera") {
            bool is_main = cpnt_desc.isMember("is_main") && cpnt_desc["is_main"].asBool();
            float near_z = cpnt_desc["near_z"].asFloat();
            float far_z = cpnt_desc["far_z"].asFloat();
            float fov = cpnt_desc["fov"].asFloat();
            std::unique_ptr<CpntCamera> camera = std::make_unique<CpntCamera>(near_z, far_z, fov);
            if (is_main) {
                camera->render_target = GL.get_rendertarget_screen();
            }
            obj->add_component(std::move(camera));
        } else if (cpnt_name == "sky_box") {
            Ref<Material> material;
            if (cpnt_desc.isMember("material")){
                material = resources.load_resource<Material>(cpnt_desc["material"].asString());
            } else {
                throw RuntimeError("天空盒必须指定材质");
            }

            Ref<GLTexture> env_map;
            if (cpnt_desc.isMember("env_map")){
                env_map = resources.load_resource<GLTexture>(cpnt_desc["env_map"].asString());
            } else {
                env_map = resources.load_resource<GLTexture>("buildin:black");
            }

            bool ignore_range = !(cpnt_desc.isMember("ignore_range") && !cpnt_desc["ignore_range"].asBool());
            BoundingBox bbox;
            if (cpnt_desc.isMember("bbox")) {
                bbox = load_bbox(cpnt_desc["bbox"]);
            } else if (!ignore_range) {
                throw RuntimeError("带范围的天空盒必须指定包围盒");
            }
            obj->add_component(std::make_unique<CpntSkybox>(material, ignore_range, bbox));
        } else {
            throw RuntimeError(std::format("未知组件：{}", cpnt_name));
        }
    }
}

// 从json递归加载节点
std::shared_ptr<GObject> load_node_from_json(const Json::Value &json) {
    const std::string &name = json.get("name", "").asString();

    std::shared_ptr<GObject> node = std::make_shared<GObject>(load_transform(json), name);

    load_conponents_from_json(node.get(), json["components"]);

    for (const Json::Value &child_desc : json["children"]) {
        node->attach_child(load_node_from_json(child_desc));
    }

    if (json.isMember("scene")) {
        // todo: copy
        Ref<Scene> sub_scene = resources.load_resource<Scene>(json["scene"].asString());
        if (sub_scene){
            node->attach_child(sub_scene->root);
        } else {
            throw RuntimeError(std::format("子场景\"{}\"加载失败",json["scene"].asString()));
        }
    }

    return node;
}

// 从json文件加载场景
Ref<Scene> load_scene_from_json(const std::string &path) {
    Ref<Scene> scene = create_ref<Scene>();
    Json::Value json;
    {
        Json::Reader reader;
        std::ifstream file(path);
        if (!file.is_open()){
            throw RuntimeError(std::format("打开文件\"{}\"失败", path));
        }
        reader.parse(file, json, false);
    }

    scene->name = json.get("name", "未命名").asString();
    // 加载物体
    scene->root = load_node_from_json(json["root"]);

    return scene;
}

} // namespace Goonya
