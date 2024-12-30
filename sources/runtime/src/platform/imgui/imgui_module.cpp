#include "imgui_module.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <imgui.h>
#include "platform/display/display.h"

namespace Goonya{

namespace ImguiMng {
    void init(){

        ImGui::SetCurrentContext(ImGui::CreateContext());
        ImGui_ImplOpenGL3_Init("#version 460");
        ImGui_ImplGlfw_InitForOpenGL(Display::window, true);
    }

    void new_frame(){
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
    }
    void render(){
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void drop(){
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }
}
}