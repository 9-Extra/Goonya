#include "display.h"
#include "runtime/GoonyaException.h"

#include "platform/encoding_cvt.h"
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

static Input::KeyCode virtual_keycode2goonya_keycode(WORD vkcode, WORD scanCode) {
    if (vkcode >= 'A' && vkcode <= 'Z') {
        return (Input::KeyCode)vkcode;
    }

    Input::KeyCode k = Input::KeyCode::UNKNOWN;

    switch (vkcode) {
        case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
        case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
        case VK_MENU:    // converts to VK_LMENU or VK_RMENU
            vkcode = LOWORD(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
    }

    switch (vkcode) {
    case VK_LSHIFT:
        k = Input::KeyCode::LSHIFT;
        break;
    case VK_RSHIFT:
        k = Input::KeyCode::RSHIFT;
        break;
    case VK_LCONTROL:
        k = Input::KeyCode::LCTRL;
        break;
    case VK_RCONTROL:
        k = Input::KeyCode::RCTRL;
        break;
    case VK_LMENU:
        k = Input::KeyCode::LALT;
        break;
    case VK_RMENU:
        k = Input::KeyCode::RALT;
        break;
    case VK_ESCAPE:
        k = Input::KeyCode::ESCAPE;
        break;
    case VK_SPACE:
        k = Input::KeyCode::SPACE;
        break;
    case VK_RETURN:
        k = Input::KeyCode::ENTER;
        break;
    case VK_TAB:
        k = Input::KeyCode::TAB;
        break;
    }

    return k;
}

HWND hwnd = NULL; // 窗口句柄
std::tuple<uint32_t, uint32_t> current_size;

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
        WORD scanCode = LOBYTE(HIWORD(lParam)); 
        Input::KeyCode vkCode = virtual_keycode2goonya_keycode(LOWORD(wParam), scanCode); // virtual-key code
        if (vkCode == Input::KeyCode::UNKNOWN) return 0;

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
    case WM_WINDOWPOSCHANGED: {
        WINDOWPOS *pos = (WINDOWPOS *)lParam;
        if ((pos->flags & SWP_NOSIZE) != SWP_NOSIZE){
            RECT rect;
            GetClientRect(hwnd, &rect);
            current_size = {rect.right - rect.left, rect.bottom - rect.top};
            EventBus::dispatch_event<true>(Events::SysDisplayResize{current_size});
            //LOG_DEBUG("窗口改变大小");
        }
        return 0;
    }
    case WM_SYSCOMMAND:{
        if ((wParam & 0xFFF0) == SC_MOVE){
            // 在结束移动时重置键盘状态（认为失去焦点）
            EventBus::dispatch_event<true>(Events::SysWindowDeActive{});
        }
        break;
    }
    case WM_DESTROY:
        LOG_DEBUG("窗口销毁", Msg);
        break;
    case WM_SETCURSOR:
    case WM_NCHITTEST:
    case WM_GETICON:
    case WM_SETTEXT:
    case WM_NCMOUSEMOVE:
    case WM_GETMINMAXINFO:
    case WM_MOVING:
    case WM_WINDOWPOSCHANGING:
    case WM_CAPTURECHANGED:
    case 0x00AE: // unknown, but ignore
        break;
    default: {
        //LOG_DEBUG("收到消息: 0x{:04x}", Msg);
    }
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

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
    const DWORD WINDOW_STYLE = WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX;
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

    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        current_size = {rect.right - rect.left, rect.bottom - rect.top};
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
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
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
    return current_size;
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