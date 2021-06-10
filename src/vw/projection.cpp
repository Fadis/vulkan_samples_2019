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

  std::tuple< glm::mat4, glm::mat4, float, float > get_aabb_light_matrix(
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
    zfar *= 0.5f;
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
    auto h = std::max( std::abs( bottom ), std::abs( top ) );
    auto light_projection_matrix = glm::frustum(
      -w, w, -h, h, znear, zfar
    );
    return std::make_tuple(
      std::move( light_projection_matrix ),
      std::move( light_view_matrix ),
      znear, zfar
    );
  }



  std::tuple< glm::mat4, glm::mat4, float, float > get_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &light_pos_,
    const glm::vec3 &camera_pos,
    float depth_offset
  ) {
    auto light_pos = light_pos_;
    auto light_pos_in_camera_projection_space = rescale_vec( camera_projection_matrix * camera_view_matrix * glm::vec4( light_pos, 1.f ) );
    std::cout << "lpcps " << light_pos_in_camera_projection_space.z << std::endl;
    //if( light_pos_in_camera_projection_space.z < 1.f ) {
    //  light_pos = camera_pos + ( light_pos - camera_pos ) / std::min( light_pos_in_camera_projection_space.z, 1.f )*2.f;
    //}
    auto world = get_projection_box( camera_projection_matrix, camera_view_matrix, light_pos_in_camera_projection_space.z );
    auto center = get_projection_center( world );
    //if( light_pos_in_camera_projection_space.z < 1.1f )
    //light_pos = glm::normalize( light_pos - center ) * 2.f/light_pos_in_camera_projection_space.z + center;
    //const auto up0 = glm::vec3( 0.f, 0.f, 1000.f );
    const auto up1 = glm::vec3( 0.f, 1000.f, 0.f );
    auto light_view_matrix =
      glm::lookAt(
        light_pos,
        center,
        up1
      )/* :
      glm::lookAt(
        light_pos,
        center,
        up0
      )*/;
    std::cout << "debug " << glm::to_string( light_pos ) << " " << glm::to_string( get_projection_center( world ) ) << std::endl;
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
    znear = std::max( znear*depth_offset, 0.1f );
    zfar = std::max( zfar, 0.2f );
    auto w = std::max( std::abs( left ), std::abs( right ) );
    auto h = std::max( std::abs( bottom ), std::abs( top ) );
    auto light_projection_matrix = glm::frustum(
      -w, w, -h, h, znear, zfar
    );
    std::cout << "camera_pos " << glm::to_string( camera_pos ) << std::endl; 
    std::cout << "znear " << znear << " zfar "  << zfar << " " << w << " " << h <<  std::endl;
    std::cout << "debug " << glm::to_string( light_pos ) << " " << glm::to_string( center ) << std::endl;
    auto light_view_matrix2 = glm::lookAt(
      light_pos,
      glm::vec3( 0.f, 0.f, 0.f ),
      up1
      /*glm::vec3( 0.f, 100.f, 0.f ),
      glm::vec3( 18.f, 7.f, -19.f ),
      glm::vec3( 0.f, 0.f, 1000.f )*/
    );
    auto light_projection_matrix2 = glm::perspective(
      std::atan( h/znear )*2, w/h, znear, zfar*1.1f
    );
    light_projection_matrix * light_view_matrix;
    light_projection_matrix2 * light_view_matrix2;
    return std::make_tuple(
      std::move( light_projection_matrix ),
      std::move( light_view_matrix ),
      znear, zfar
    );
  }

  std::pair< glm::mat4, glm::mat4 > get_perspective_light_matrix(
    const glm::mat4 &camera_projection_matrix,
    const glm::mat4 &camera_view_matrix,
    const glm::vec3 &light_pos,
    float depth_offset
  ) {
    auto world = std::array< glm::vec3, 8u >{
      glm::vec3( -1, -1, -1 ),
      glm::vec3( 1, -1, -1 ),
      glm::vec3( -1, 1, -1 ),
      glm::vec3( 1, 1, -1 ),
      glm::vec3( -1, -1, 1 ),
      glm::vec3( 1, -1, 1 ),
      glm::vec3( -1, 1, 1 ),
      glm::vec3( 1, 1, 1 )
    };
    const glm::vec3 &light_pos_in_screen_space =
      camera_projection_matrix * camera_view_matrix * glm::vec4( light_pos, 1.f );
    auto light_view_matrix = glm::lookAt(
      light_pos_in_screen_space,
      glm::vec3{ 0.f, 0.f, 0.f },
      glm::vec3( 0.f, 100.f, 0.f )
    );
    for( auto &v: world ) v = rescale_vec( light_view_matrix * glm::vec4( v, 1.f ) );
    float znear = std::numeric_limits< float >::max();
    float zfar = std::numeric_limits< float >::min();
    for( const auto &v: world ) {
      if( zfar < -v[ 2 ] ) zfar = -v[ 2 ];
      if( znear > -v[ 2 ] ) znear = -v[ 2 ];
    }
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
    auto light_projection_matrix = glm::frustum(
      left, right, bottom, top, ( znear*depth_offset ), zfar
    );
    return std::make_pair(
      std::move( light_projection_matrix ),
      std::move( light_view_matrix )
    );
  }
}
