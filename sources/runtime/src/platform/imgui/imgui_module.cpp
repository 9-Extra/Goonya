#include "imgui_module.h"

#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include "platform/display/display.h"

namespace Goonya{

namespace ImguiMng {
    void init(){

        ImGui::SetCurrentContext(ImGui::CreateContext());
        Display::init_for_imgui();
        ImGui_ImplOpenGL3_Init("#version 460");
    }

    void new_frame(){
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
    }
    void render(){
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void drop(){
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
    }
}
}