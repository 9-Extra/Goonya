#include "display.h"
#include "core/input/input.h"
#include "runtime/GoonyaException.h"

#include <cassert>
#include <cstdint>
#include <string>

#include "core/eventbus/eventbus.h"
#include "core/log/Log.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui_impl_glfw.h>
#include <utility>


namespace Goonya {
namespace Display {

GLFWwindow *window;

static Input::KeyCode glfw_key2goonya_keycode(int key) {

    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z){
        return Input::KeyCode(key);
    }

    Input::KeyCode k = Input::KeyCode::UNKNOWN;

    switch (key) {
    case GLFW_KEY_LEFT_SHIFT:
        k = Input::KeyCode::LSHIFT;
        break;
    case GLFW_KEY_RIGHT_SHIFT:
        k = Input::KeyCode::RSHIFT;
        break;
    case GLFW_KEY_LEFT_CONTROL:
        k = Input::KeyCode::LCTRL;
        break;
    case GLFW_KEY_RIGHT_CONTROL:
        k = Input::KeyCode::RCTRL;
        break;
    case GLFW_KEY_LEFT_ALT:
        k = Input::KeyCode::LALT;
        break;
    case GLFW_KEY_RIGHT_ALT:
        k = Input::KeyCode::RALT;
        break;
    case GLFW_KEY_ESCAPE:
        k = Input::KeyCode::ESCAPE;
        break;
    case GLFW_KEY_SPACE:
        k = Input::KeyCode::SPACE;
        break;
    case GLFW_KEY_ENTER:
        k = Input::KeyCode::ENTER;
        break;
    case GLFW_KEY_TAB:
        k = Input::KeyCode::TAB;
        break;
    }

    return k;
}

static Input::KeyState glfw_action2keystate(int action){
    return action == GLFW_RELEASE ? Input::KeyState::UP : Input::KeyState::DOWN;
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_REPEAT)
        return;

    Input::KeyCode vkCode = glfw_key2goonya_keycode(key); // virtual-key code
    if (vkCode == Input::KeyCode::UNKNOWN)
        return;
    
    EventBus::dispatch_event<true>(Events::SysKeyEvent{vkCode, glfw_action2keystate(action)});
    
}


static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // The action is one of GLFW_PRESS or GLFW_RELEASE，不会有GLFW_REPEAT
    Input::MouseKey key = Input::MouseKey::LEFT;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            key = Input::MouseKey::LEFT;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            key = Input::MouseKey::MIDDLE;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            key = Input::MouseKey::RIGHT;
            break;
        default:
            std::unreachable();
    }

    EventBus::dispatch_event(Events::SysMouseClick{key, glfw_action2keystate(action)});
    
}

static void glfw_cursor_position_callbackc(GLFWwindow* window, double xpos, double ypos){
    EventBus::dispatch_event<true>(Events::SysMousePos{xpos, ypos});
}

static void glfw_window_close_callback(GLFWwindow* window){
    LOG_DEBUG("收到关闭消息");
    EventBus::dispatch_event<true>(Events::SysWindowClose{});
}

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    EventBus::dispatch_event<true>(Events::SysDisplayResize{{(uint32_t) width, (uint32_t)height}});
    LOG_DEBUG("客户区改变大小({}, {})", width, height);
}

void glfw_window_focus_callback(GLFWwindow* window, int focused)
{
    if (focused)
    {
        // The window gained input focus
    }
    else
    {
        EventBus::dispatch_event<true>(Events::SysWindowDeActive{});
    }
}

void create_opengl_context() { glfwMakeContextCurrent(window); }

void drop_opengl_context() {
    // no need for glfw
}

void swap() { glfwSwapBuffers(window); }

void create_window(uint32_t width, uint32_t height) {
    // window setting
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    // opengl setting
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, true);
    glfwWindowHint(GLFW_SRGB_CAPABLE, false);
    glfwWindowHint(GLFW_AUX_BUFFERS, false);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    
#ifdef DEBUG
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
#endif
    window = glfwCreateWindow(width, height, "GoonyaWindow", NULL, NULL);
    if (!window) {
        throw RuntimeError("创建窗口失败");
    }
}

void static glfw_error_callback(int error, const char *description) {
    LOG_ERROR("GLFW Error {}: {}", error, description);
}

void initalize(uint32_t width, uint32_t height) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        throw RuntimeError("GLFW init error");
    }

    create_window(width, height);
    // if (glfwRawMouseMotionSupported()){
    //     glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    // }
    create_opengl_context();

    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetWindowCloseCallback(window, glfw_window_close_callback);
    glfwSetWindowFocusCallback(window, glfw_window_focus_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_position_callbackc);

    glfwSwapInterval(1); // 垂直同步
}

void drop() {
    drop_opengl_context();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void set_title(const std::string &title) { glfwSetWindowTitle(window, title.c_str()); }

std::tuple<uint32_t, uint32_t> get_size() { 
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return {w, h};
}

void poll_events() { glfwPollEvents(); }

} // namespace Display
} // namespace Goonya