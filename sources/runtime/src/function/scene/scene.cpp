#include "scene.h"

#include <fstream>
#include <json/json.h>
#include "function/graphics/components/CpntMeshRender.h"
#include "function/graphics/components/CpntPointLight.h"
#include "function/graphics/components/CpntCamera.h"
#include "runtime/GoonyaException.h"

namespace Goonya {
namespace Scene {
// 从json加载一个Vector3f
Vector3f load_vec3(const Json::Value &json) {
    assert(json.isArray());
    return {json[0].asFloat(), json[1].asFloat(), json[2].asFloat()};
}
// 从json加载transform，对不完整或不存在的取默认值
Transform load_transform(const Json::Value &json) {
    Transform trans{
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
    };
    if (json.isMember("transform")) {
        const Json::Value &t = json["transform"];
        if (t.isMember("position")) {
            trans.position = load_vec3(t["position"]);
        }
        if (t.isMember("rotation")) {
            trans.rotation = load_vec3(t["rotation"]);
        }
        if (t.isMember("scale")) {
            trans.scale = load_vec3(t["scale"]);
        }
    }

    return trans;
}

// 从json加载组件
void load_conponents_from_json(GObject *obj, const Json::Value &json) {
    for (const auto& cpnt_name :json.getMemberNames()) {
        const Json::Value cpnt_desc = json[cpnt_name];
        if (cpnt_name == "mesh_render") {
            std::unique_ptr<Graphics::CpntMeshRender> cpnt_ptr = std::make_unique<Graphics::CpntMeshRender>();
            if (cpnt_desc.isMember("parts")){
                for(const Json::Value &p : cpnt_desc["parts"]) {
                    cpnt_ptr->add_part(Graphics::RenderItem{p["mesh"].asString(), p["material"].asString(), GL_TRIANGLES, load_transform(p)});
                }
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
            obj->add_component(std::make_unique<Graphics::CpntCamera>(near_z, far_z, fov));  
            if (is_main){
                obj->get_component<Graphics::CpntCamera>()->set_main_camera();
            }
        } else {
            std::cout << "unknown component: " << cpnt_name << std::endl;
        }
    }
}

// 从json递归加载节点
void load_node_from_json(const Json::Value &node, GObject *root) {
    for (const Json::Value &object_desc: node) {
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
    // 加载天空盒
    if (!json.isMember("skybox")) {
        throw Goonya::RuntimeError("Skybox is required for a scene");
    }

    Graphics::renderer.set_skybox(json["skybox"]["specular_texture"].asString());
    // 加载物体
    if (json.isMember("root")) {
        load_node_from_json(json["root"], scene.root.get());
    }

    return scene;
}

}
}