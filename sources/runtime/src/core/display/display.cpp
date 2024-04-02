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

namespace Goonya {

LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

namespace Display {

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
    if (!SetPixelFormat(hdc, pixel_format, &pfd)){
        throw Goonya::RuntimeError("设置颜色格式失败");
    }

    HGLRC hrc = wglCreateContext(hdc);

    if (hrc == NULL){
        throw Goonya::RuntimeError("创建Opengl上下文失败");
    }

    wglMakeCurrent(hdc, hrc);
}

void drop_opengl_context() {
    HGLRC hrc = wglGetCurrentContext();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hrc);
}

void swap() { 
    if (!wglSwapLayerBuffers(GetDC(hwnd), WGL_SWAP_MAIN_PLANE)){
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