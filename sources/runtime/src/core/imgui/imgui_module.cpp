#include "imgui_module.h"

#include <imgui_impl_win32.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <windows.h>

namespace Goonya{


namespace Display {
extern HWND hwnd; // 窗口句柄
}

namespace ImguiMng {
    void init(){

        ImGui::SetCurrentContext(ImGui::CreateContext());
        ImGui_ImplWin32_InitForOpenGL(Display::hwnd);
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