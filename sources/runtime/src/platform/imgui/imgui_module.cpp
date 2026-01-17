#include "imgui_module.h"

#include "platform/display/display.h"
#include "platform/graphics/Graphics.h"
#include "runtime/GAssert.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Goonya::ImguiMng {

static bool initialized = false;
void init() {
    GN_ASSERT_MSG(!initialized, "Imgui模块重复初始化");
    initialized = true;
    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplOpenGL3_Init("#version 460");
    ImGui_ImplGlfw_InitForOpenGL(Display::window, true);
}

void new_frame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
}
void render() {
    GL.get_rendertarget_screen()->bind_draw();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void drop() {
    if (!initialized) {
        return;
    }
    initialized = false;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}
} // namespace Goonya::ImguiMng
