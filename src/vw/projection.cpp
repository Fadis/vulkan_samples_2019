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
#include <iostream>
#include <vw/projection.h>
#include <cmath>
#include <array>
#include <numeric>
#include <tuple>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/string_cast.hpp>
namespace vw {
  glm::vec3 rescale_vec( const glm::vec4 &v ) {
    return glm::vec3( v[ 0 ]/v[ 3 ], v[ 1 ]/v[ 3 ], v[ 2 ]/v[ 3 ] );
  }

  float logarithmic_split(
    float i,
    float count,
    float near,
    float far
  ) {
    return near * std::pow( ( far / near ), ( i / count ) );
  }

  float practical_split(
    float i,
    float count,
    float near,
    float far,
    float a
  ) {
   return a * logarithmic_split( i, count, near, far ) + ( 1.f - a ) * ( near + i / count * ( far - near ) );
  }

  std::array< glm::vec3, 8u > get_projection_box(
    glm::mat4 projection_matrix,
    glm::mat4 camera_matrix,
    float /*light_distance*/
  ) {
    glm::mat4 combined_matrix = projection_matrix * camera_matrix;
    glm::mat4 icm = glm::inverse( combined_matrix );
    const float depth_limit = 1.f/*std::max( std::min( light_distance, 1.f ), 0.1f )*/;
    return std::array< glm::vec3, 8u >{
      rescale_vec( icm * glm::vec4( -1, -1, -depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( 1, -1, -depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( -1, 1, -depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( 1, 1, -depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( -1, -1, depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( 1, -1, depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( -1, 1, depth_limit, 1 ) ),
      rescale_vec( icm * glm::vec4( 1, 1, depth_limit, 1 ) )
    };
  }

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_aabb_light_matrix(
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos
  ) {
    std::array< glm::vec3, 8u > world{
      glm::vec3( min[ 0 ], min[ 1 ], min[ 2 ] ),
      glm::vec3( max[ 0 ], min[ 1 ], min[ 2 ] ),
      glm::vec3( min[ 0 ], max[ 1 ], min[ 2 ] ),
      glm::vec3( max[ 0 ], max[ 1 ], min[ 2 ] ),
      glm::vec3( min[ 0 ], min[ 1 ], max[ 2 ] ),
      glm::vec3( max[ 0 ], min[ 1 ], max[ 2 ] ),
      glm::vec3( min[ 0 ], max[ 1 ], max[ 2 ] ),
      glm::vec3( max[ 0 ], max[ 1 ], max[ 2 ] )
    };
    auto center = ( min + max ) / 2.f;
    auto up1 = glm::vec3( 0.f, 1000.f, 0.f );
    if( glm::dot( glm::normalize( light_pos - center ), glm::normalize( up1 - center ) ) > 0.8f )
      up1 = glm::vec3( 0.f, 0.f, 1000.f );
    auto light_view_matrix =
      glm::lookAt(
        light_pos,
        center,
        up1
      );
    for( auto &v: world ) v = rescale_vec( light_view_matrix * glm::vec4( v, 1.f ) );
    float znear = std::numeric_limits< float >::max();
    float zfar = std::numeric_limits< float >::min();
    for( const auto &v: world ) {
      if( zfar < -v[ 2 ] ) zfar = -v[ 2 ];
      if( znear > -v[ 2 ] ) znear = -v[ 2 ];
    }
    znear *= 0.5f;
    for( auto &v: world ) v = v*( -znear/v[ 2 ] );
    float left = std::numeric_limits< float >::max();
    float right = std::numeric_limits< float >::min();
    float bottom = std::numeric_limits< float >::max();
    float top = std::numeric_limits< float >::min();
    for( const auto &v: world ) {
      if( right < v[ 0 ] ) right = v[ 0 ];
      if( left > v[ 0 ] ) left = v[ 0 ];
      if( top < v[ 1 ] ) top = v[ 1 ];
      if( bottom > v[ 1 ] ) bottom = v[ 1 ];
    }
    auto w = std::max( std::abs( left ), std::abs( right ) );
    auto light_projection_matrix = glm::frustum(
      left, right, bottom, top, znear, zfar
    );
    return std::make_tuple(
      std::move( light_projection_matrix ),
      std::move( light_view_matrix ),
      znear, zfar, w*2
    );
  }

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_projection_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos,
    float distance_offset
  ) {
    auto projection_box = get_projection_box( camera_projection_matrix, camera_view_matrix, distance_offset );
    glm::vec3 proj_min = max;
    glm::vec3 proj_max = min;
    for( auto v: projection_box ) {
      if( v.x < proj_min.x ) proj_min.x = std::max( v.x, min.x );
      if( v.x > proj_max.x ) proj_max.x = std::min( v.x, max.x );
      if( v.y < proj_min.y ) proj_min.y = std::max( v.y, min.y );
      if( v.y > proj_max.y ) proj_max.y = std::min( v.y, max.y );
      if( v.z < proj_min.z ) proj_min.z = std::max( v.z, min.z );
      if( v.z > proj_max.z ) proj_max.z = std::min( v.z, max.z );
    }
    return get_aabb_light_matrix( proj_min, proj_max, light_pos ); 
  }

  glm::mat4 get_l(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &light_projection_matrix,
    const glm::mat4 &light_view_matrix
  ) {
    auto icam = glm::inverse( camera_projection_matrix * camera_view_matrix );
    auto cam_near = light_projection_matrix * light_view_matrix * icam * glm::vec4( 0, 0, 1, 1 );
    auto cam_far = light_projection_matrix * light_view_matrix * icam * glm::vec4( 0, 0, -1, 1 );
    glm::vec3 cam_dir = rescale_vec( cam_near ) - rescale_vec( cam_far );
    glm::vec2 rot( cam_dir[ 0 ], cam_dir[ 1 ] );
    rot = glm::normalize( rot );
    glm::mat4 rotm(
       rot[ 1 ], -rot[ 0 ], 0, 0,
       rot[ 0 ],  rot[ 1 ], 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
    );
    return rotm;
  }

  glm::mat4 get_w(
    float near
  ) {
    float far = near + 2.0f;
    glm::mat4 i = glm::mat4( 1 );
    float center = ( near + far ) / 2.f;
    glm::mat4 wt = glm::translate( i, glm::vec3( 0.f, near, 0.f ) );
    glm::mat4 wv = glm::scale( wt, glm::vec3( near / center ) );
    glm::mat4 wp(0);
    wp[0][0] = near;
    wp[1][1] = far / ( far - near );
    wp[2][2] = near;
    wp[1][3] = 1;
    wp[3][1] = - ( far * near ) / ( far - near );
    return wp * wv;
  }

  std::tuple< glm::vec3, glm::vec3 > get_aabb_in_screen_space(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &min,
    const glm::vec3 &max
  ) {
    std::array< glm::vec3, 8u > world{
      glm::vec3( min[ 0 ], min[ 1 ], min[ 2 ] ),
      glm::vec3( max[ 0 ], min[ 1 ], min[ 2 ] ),
      glm::vec3( min[ 0 ], max[ 1 ], min[ 2 ] ),
      glm::vec3( max[ 0 ], max[ 1 ], min[ 2 ] ),
      glm::vec3( min[ 0 ], min[ 1 ], max[ 2 ] ),
      glm::vec3( max[ 0 ], min[ 1 ], max[ 2 ] ),
      glm::vec3( min[ 0 ], max[ 1 ], max[ 2 ] ),
      glm::vec3( max[ 0 ], max[ 1 ], max[ 2 ] )
    };
    for( auto v: world ) {
      auto p = camera_projection_matrix * camera_view_matrix * glm::vec4( v[ 0 ], v[ 1 ], v[ 2 ], 1.0f );
      v = glm::vec3( p[ 0 ], p[ 1 ], p[ 2 ] );
    }
    glm::vec3 proj_min = world[ 0 ];
    glm::vec3 proj_max = world[ 0 ];
    for( auto v: world ) {
      if( v.x < proj_min.x ) proj_min.x = v.x;
      if( v.x > proj_max.x ) proj_max.x = v.x;
      if( v.y < proj_min.y ) proj_min.y = v.y;
      if( v.y < proj_max.y ) proj_max.y = v.y;
      if( v.z < proj_min.z ) proj_min.z = v.z;
      if( v.z < proj_max.z ) proj_max.z = v.z;
    }
    return std::make_tuple( proj_min, proj_max );
  }

  std::tuple< glm::mat4, glm::mat4, float, float, float > get_perspective_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &min,
    const glm::vec3 &max,
    const glm::vec3 &light_pos
  ) {
    auto [screen_min,screen_max] = get_aabb_in_screen_space( camera_projection_matrix, camera_view_matrix, min, max );
    auto screen_projection_box = std::array< glm::vec3, 8u >{
      glm::vec3( -1, -1, -1 ),
      glm::vec3( 1, -1, -1 ),
      glm::vec3( -1, 1, -1 ),
      glm::vec3( 1, 1, -1 ),
      glm::vec3( -1, -1, 1 ),
      glm::vec3( 1, -1, 1 ),
      glm::vec3( -1, 1, 1 ),
      glm::vec3( 1, 1, 1 )
    };
    glm::vec3 proj_min = screen_max;
    glm::vec3 proj_max = screen_min;
    for( auto v: screen_projection_box ) {
      if( v.x < proj_min.x ) proj_min.x = std::max( v.x, min.x );
      if( v.x > proj_max.x ) proj_max.x = std::min( v.x, max.x );
      if( v.y < proj_min.y ) proj_min.y = std::max( v.y, min.y );
      if( v.y > proj_max.y ) proj_max.y = std::min( v.y, max.y );
      if( v.z < proj_min.z ) proj_min.z = std::max( v.z, min.z );
      if( v.z > proj_max.z ) proj_max.z = std::min( v.z, max.z );
    }
    auto light_pos_in_camera_projection_space = camera_projection_matrix * camera_view_matrix * glm::vec4( light_pos, 1.f );
    auto light_pos_in_camera_projection_space_scaled = rescale_vec( light_pos_in_camera_projection_space );
    return get_aabb_light_matrix( proj_min, proj_max, light_pos_in_camera_projection_space_scaled );
  }


  std::tuple< glm::mat4, glm::mat4, float, float, float > get_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &light_pos_,
    const glm::vec3 &/*camera_pos*/,
    float depth_offset
  ) {
    auto light_pos = light_pos_;
    auto light_pos_in_camera_projection_space = rescale_vec( camera_projection_matrix * camera_view_matrix * glm::vec4( light_pos, 1.f ) );
    auto world = get_projection_box( camera_projection_matrix, camera_view_matrix, light_pos_in_camera_projection_space.z );
    auto center = get_projection_center( world );
    const auto up1 = glm::vec3( 0.f, 1000.f, 0.f );
    auto light_view_matrix =
      glm::lookAt(
        light_pos,
        center,
        up1
      );
    for( auto &v: world ) v = rescale_vec( light_view_matrix * glm::vec4( v, 1.f ) );
    float znear = std::numeric_limits< float >::max();
    float zfar = std::numeric_limits< float >::min();
    for( const auto &v: world ) {
      if( zfar < -v[ 2 ] ) zfar = -v[ 2 ];
      if( znear > -v[ 2 ] ) znear = -v[ 2 ];
    }
    znear *= 0.5f;
    zfar *= 2.0f;
    for( auto &v: world ) v = v*( -( znear*depth_offset )/v[ 2 ] );
    float left = std::numeric_limits< float >::max();
    float right = std::numeric_limits< float >::min();
    float bottom = std::numeric_limits< float >::max();
    float top = std::numeric_limits< float >::min();
    for( const auto &v: world ) {
      if( right < v[ 0 ] ) right = v[ 0 ];
      if( left > v[ 0 ] ) left = v[ 0 ];
      if( top < v[ 1 ] ) top = v[ 1 ];
      if( bottom > v[ 1 ] ) bottom = v[ 1 ];
    }
    auto w = std::max( std::abs( left ), std::abs( right ) );
    auto light_projection_matrix = glm::frustum(
      left, right, bottom, top, znear, zfar
    );
    return std::make_tuple(
      std::move( light_projection_matrix ),
      std::move( light_view_matrix ),
      znear, zfar, w*2
    );
  }
}
