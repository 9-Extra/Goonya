#include "imgui_module.h"

#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "platform/display/display.h"
#include "platform/graphics/Graphics.h"
#include "resource/ResMng.h"
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

    // 加载中文字体
    auto font_path = resources.get_root_dir() / "fonts/HarmonyOS_Sans_SC_Regular.ttf";

    ImGuiIO &io = ImGui::GetIO();
    ImFont *font = io.Fonts->AddFontFromFileTTF((const char *)font_path.u8string().c_str(), 18.0f, nullptr,
                                                io.Fonts->GetGlyphRangesChineseFull());
    if (font == nullptr) {
        LOG_ERROR("字体加载失败: {}，回退到默认字体", font_path);
    } else {
        LOG_INFO("Imgui使用字体: {}", font_path);
    }

    ImGui_ImplOpenGL3_Init("#version 460");
    ImGui_ImplGlfw_InitForOpenGL(Display::window, true);
}

void new_frame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
}
void render() {
    GL.get_rendertarget_screen()->bind_draw();
    GL.push_debug_group_label("Imgui");
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    GL.pop_debug_group_label();
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
