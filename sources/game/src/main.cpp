#include <exception>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <runtime/Goonya.h>
#include <runtime/log/Log.h>
#include <filesystem>
#include <Windows.h>

std::filesystem::path get_exe_path(){
    wchar_t szPath[512] = {0};
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) - 1);
    return std::filesystem::path(szPath).remove_filename();
}

int main() {
    try {
        Goonya::init_engine();
        try {
            Json::Value root;
            std::ifstream(get_exe_path() / "../assets/scene1.json") >> root;
            const Json::Value &pointlight = root["pointlights"][0];
            for (const std::string &name : pointlight.getMemberNames()) {
                std::cout << name << std::endl;
            }
        } catch (const Json::Exception &e) {
            LOG_ERROR(e.what());
        }

        Goonya::main_loop();

        Goonya::drop_engine();
    } catch (const std::exception &e) {
        LOG_ERROR(e.what());
    }

    std::cerr << "正常关闭" << std::endl;

    return 0;
}