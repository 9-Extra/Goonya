#include "display.h"
#include "runtime/GoonyaException.h"

#include "utils/encoding_cvt.h"
#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <format>
#include <hidusage.h>
#include <string>
#include <wingdi.h>

#include <imgui_impl_win32.h>

#include "core/eventbus/eventbus.h"
#include "runtime/log/Log.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Goonya {
namespace Display {

LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) {
        return 0;
    }

    switch (Msg) {
    case WM_INPUT: {
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        RAWINPUT *raw = (RAWINPUT *)new BYTE[dwSize];
        if (raw == NULL) {
            return 0;
        }

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            LOG_ERROR("GetRawInputData doesn't return correct size !");
            return 0;
        }

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            RAWMOUSE &mouse = raw->data.mouse;
            USHORT button_state = mouse.usButtonFlags;
            Events::SysMouseClick click_event;
            bool is_clicked = true;
            switch (button_state) {
            case RI_MOUSE_LEFT_BUTTON_DOWN:
                click_event.key = Input::MOUSEKEY::LEFT;
                click_event.up_down = true;
                break;
            case RI_MOUSE_LEFT_BUTTON_UP:
                click_event.key = Input::MOUSEKEY::LEFT;
                click_event.up_down = false;
                break;
            case RI_MOUSE_RIGHT_BUTTON_DOWN:
                click_event.key = Input::MOUSEKEY::RIGHT;
                click_event.up_down = true;
                break;
            case RI_MOUSE_RIGHT_BUTTON_UP:
                click_event.key = Input::MOUSEKEY::RIGHT;
                click_event.up_down = false;
                break;
            case RI_MOUSE_MIDDLE_BUTTON_DOWN:
                click_event.key = Input::MOUSEKEY::MIDDLE;
                click_event.up_down = true;
                break;
            case RI_MOUSE_MIDDLE_BUTTON_UP:
                click_event.key = Input::MOUSEKEY::MIDDLE;
                click_event.up_down = false;
                break;
            default:
                is_clicked = false;
            }
            if (is_clicked) {
                EventBus::dispatch_event(click_event);
            }
            EventBus::dispatch_event<true>(Events::SysRawMouseMove{mouse.lLastX, mouse.lLastY});
        }
        delete[] raw;
        return 0;
    }
    case WM_KEYUP:
    case WM_KEYDOWN: {
        WORD vkCode = LOWORD(wParam); // virtual-key code
        WORD keyFlags = HIWORD(lParam);
        if (Msg == WM_KEYDOWN) {
            bool is_repeat = (keyFlags & KF_REPEAT) != 0;
            if (is_repeat) {
                return 0;
            }
            EventBus::dispatch_event<true>(Events::SysKeyEvent{vkCode, true});
        } else {
            EventBus::dispatch_event<true>(Events::SysKeyEvent{vkCode, false});
        }
        return 0;
    }
    case WM_ACTIVATE: {
        if (wParam == WA_INACTIVE) {
            EventBus::dispatch_event<true>(Events::SysWindowDeActive{});
        }
        return 0;
    }
    case WM_CHAR:
        return 0;
    case WM_CLOSE:
        LOG_DEBUG("收到关闭消息", Msg);
        EventBus::dispatch_event<true>(Events::SysWindowClose{});
        return 0;
    case WM_MOUSEMOVE: {
        int32_t xPos = lParam & 0xffff;
        int32_t yPos = lParam >> 16 & 0xffff;
        EventBus::dispatch_event<true>(Events::SysMousePos{xPos, yPos});
        return 0;
    }
    case WM_DESTROY:
        LOG_DEBUG("窗口销毁", Msg);
        break;
    case WM_SETCURSOR:
    case WM_NCHITTEST:
        break;
    default: {
        // LOG_DEBUG("收到消息: 0x{:04x}", Msg);
    }
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}
HWND hwnd = NULL; // 窗口句柄

void create_opengl_context() {

    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                                 PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
                                 32,            // Colordepth of the framebuffer.
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24, // Number of bits for the depthbuffer
                                 8,  // Number of bits for the stencilbuffer
                                 0,  // Number of Aux buffers in the framebuffer.
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if (!SetPixelFormat(hdc, pixel_format, &pfd)) {
        throw Goonya::RuntimeError("设置颜色格式失败");
    }

    HGLRC hrc = wglCreateContext(hdc);

    if (hrc == NULL) {
        throw Goonya::RuntimeError("创建Opengl上下文失败");
    }

    wglMakeCurrent(hdc, hrc);
}

void init_for_imgui() { ImGui_ImplWin32_InitForOpenGL(Display::hwnd); }

void drop_opengl_context() {
    HGLRC hrc = wglGetCurrentContext();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);
}

void swap() {
    if (!wglSwapLayerBuffers(GetDC(hwnd), WGL_SWAP_MAIN_PLANE)) {
        throw Goonya::RuntimeError("交换前后缓冲失败");
    }
}

void create_window(uint32_t width, uint32_t height) {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    const wchar_t window_class_name[] = L"GoonyaWindow";
    const DWORD WINDOW_STYLE = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    const WNDCLASSEXW window_class = {.cbSize = sizeof(WNDCLASSEXW),
                                      .style = CS_OWNDC,
                                      .lpfnWndProc = WindowProc,
                                      .cbClsExtra = 0,
                                      .cbWndExtra = 0,
                                      .hInstance = hInstance,
                                      .hIcon = LoadIcon(NULL, IDI_WINLOGO),
                                      .hCursor = LoadCursor(NULL, IDC_ARROW),
                                      .hbrBackground = NULL,
                                      .lpszMenuName = NULL,
                                      .lpszClassName = window_class_name,
                                      .hIconSm = NULL};

    RegisterClassExW(&window_class);
    RECT rect = {.left = 100, .top = 100, .right = 100 + (LONG)width, .bottom = 100 + (LONG)height};
    AdjustWindowRectEx(&rect, WINDOW_STYLE, NULL, 0);

    hwnd = CreateWindowExW(0, window_class_name, L"Goonya", WINDOW_STYLE, 0, 0, rect.right - rect.left,
                           rect.bottom - rect.top, NULL, NULL, hInstance, nullptr);

    if (hwnd == NULL) {
        throw RuntimeError(std::format("创建窗口失败: {}", GetLastError()));
    }
}

void register_raw_input() {
    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    Rid[0].dwFlags = 0;
    Rid[0].hwndTarget = hwnd;
    if (RegisterRawInputDevices(Rid, 1, sizeof(RAWINPUTDEVICE)) == FALSE) {
        throw RuntimeError(std::format("注册原始输入设备失败: {}", GetLastError()));
    }
}

void initalize(uint32_t width, uint32_t height) {
    create_window(width, height);
    create_opengl_context();
    register_raw_input();

    ShowWindow(hwnd, SW_SHOW);
}

void drop() {
    drop_opengl_context();
    assert(hwnd != NULL);
    DestroyWindow(hwnd);
    hwnd = NULL;
}

void set_title(const std::string &title) { SetWindowTextW(hwnd, utf8_to_wchar(title).c_str()); }

std::tuple<uint32_t, uint32_t> get_size() {
    RECT rect;
    GetClientRect(hwnd, &rect);
    return {rect.right - rect.left, rect.bottom - rect.top};
}
void poll_events() {
    MSG msg;
    while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

} // namespace Display
} // namespace Goonya