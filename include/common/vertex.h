#ifndef VULKAN_SAMPLES_INCLUDE_VERTEX_H
#define VULKAN_SAMPLES_INCLUDE_VERTEX_H
#include <glm/glm.hpp>
namespace common {
  struct vertex_t {
    vertex_t() = default;
    vertex_t(
      glm::f32vec3 position_,
      glm::f32vec3 normal_,
      glm::f32vec3 tangent_,
      glm::f32vec2 texcoord_
    ) :
      position( position_ ),
      normal( normal_ ),
      tangent( tangent_ ),
      texcoord( texcoord_ ) {}
    glm::f32vec3 position;
    glm::f32vec3 normal;
    glm::f32vec3 tangent;
    glm::f32vec2 texcoord;
  };
}
#endif




