#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <common/draw_state.h>
namespace common {
  void on_key_event( GLFWwindow *window, int key, int, int action, int ) {
    draw_state_t &draw_state = *reinterpret_cast< draw_state_t* >( glfwGetWindowUserPointer( window ) );
    if( key == GLFW_KEY_Q && action == GLFW_PRESS ) draw_state.end = true;
  }
}

