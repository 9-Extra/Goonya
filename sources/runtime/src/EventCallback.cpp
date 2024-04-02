#include "EventCallback.h"
#include "core/input/input_inner_interface.h"
#include "runtime/log/Log.h"

#include <Windows.h>

namespace Goonya {

bool should_exit = false;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
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
            switch (button_state){
                case RI_MOUSE_LEFT_BUTTON_DOWN:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::LEFT] = true;
                    break;
                case RI_MOUSE_LEFT_BUTTON_UP:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::LEFT] = false;
                    break;
                case RI_MOUSE_RIGHT_BUTTON_DOWN:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::RIGHT] = true;
                    break;
                case RI_MOUSE_RIGHT_BUTTON_UP:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::RIGHT] = false;
                    break;
                case RI_MOUSE_MIDDLE_BUTTON_DOWN:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::MIDDLE] = true;
                    break;
                case RI_MOUSE_MIDDLE_BUTTON_UP:
                    Input::Detail::mouse_key_state[Input::MOUSEKEY::MIDDLE] = false;
                    break;
            }
            Input::Detail::on_mouse_move(mouse.lLastX, mouse.lLastY);
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
            LOG_WARN("按下键: {}", vkCode);
            Input::Detail::on_key_down(vkCode);
        } else {
            LOG_WARN("松开键: {}", vkCode);
            Input::Detail::on_key_up(vkCode);
        }
        return 0;
    }
    case WM_ACTIVATE: {
        if (wParam == WA_INACTIVE) {
            Input::reset_state();
        }
        return 0;
    }
    case WM_CHAR:
        return 0;
    case WM_CLOSE:
        LOG_DEBUG("收到关闭消息", Msg);
        ShowWindow(hWnd, SW_HIDE);
        should_exit = true;
        return 0;
    case WM_MOUSEMOVE: {
        int32_t xPos = lParam & 0xffff;
        int32_t yPos = lParam >> 16 & 0xffff;
        Input::Detail::on_mouse_set(xPos, yPos);
        return 0;
    }
    case WM_SETCURSOR:
    case WM_NCHITTEST:
        break;
    default:{
        //LOG_DEBUG("收到消息: 0x{:04x}", Msg);
    }
        
    }
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}
}