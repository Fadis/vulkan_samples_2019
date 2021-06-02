#ifndef VW_PROJECTION_H
#define VW_PROJECTION_H
/*
 * Copyright (C) 2020 Naomasa Matsubayashi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <vw/projection.h>
#include <utility>
#include <array>
#include <numeric>
#include <tuple>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
namespace vw {
  glm::vec3 rescale_vec( const glm::vec4 &v );

  std::array< glm::vec3, 8u > get_projection_box(
    glm::mat4 projection_matrix,
    glm::mat4 camera_matrix,
    float
  );

  template< typename R >
  glm::vec3 get_projection_center( const R &range ) {
    return std::accumulate(
      range.begin(), range.end(),
      glm::vec3( 0 )
    ) / float(std::distance( range.begin(), range.end() ) );
  }

  float logarithmetic_split(
    float i,
    float count,
    float near,
    float far
  );
  float practical_split(
    float i,
    float count,
    float near,
    float far,
    float a
  );
  
  std::tuple< glm::mat4, glm::mat4, float, float, float > get_aabb_light_matrix(
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos
  );

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &light_pos,
    const glm::vec3 &camera_pos,
    float depth_offset
  );

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_projection_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos,
    float
  );
  
  glm::mat4 get_l(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &light_projection_matrix,
    const glm::mat4 &light_view_matrix
  );

  glm::mat4 get_w(
    float near
  );

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_perspective_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos
  );
}

#endif

