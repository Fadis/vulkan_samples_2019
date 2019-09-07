#ifndef VULKAN_SAMPLES_INCLUDE_DRAW_STATE_H
#define VULKAN_SAMPLES_INCLUDE_DRAW_STATE_H
namespace common {
  struct draw_state_t {
    draw_state_t() : end( false ) {}
    bool end;
  };
  void on_key_event( GLFWwindow *window, int key, int, int action, int );
}
#endif
