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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vw/node.h>
namespace vw {
  glm::tmat4x4< float > to_matrix(
    const std::array< float, 3 > &t,
    const std::array< float, 4 > &r,
    const std::array< float, 3 > &s
  ) {
    glm::tmat4x4< float > m( 1.0f );
    auto trans = glm::tmat4x4< float >{
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      -t[ 0 ], t[ 1 ], t[ 2 ], 1.f,
    };
    auto scale = glm::tmat4x4< float >{
      -s[ 0 ], 0.f, 0.f, 0.f,
      0.f, s[ 1 ], 0.f, 0.f,
      0.f, 0.f, s[ 2 ], 0.f,
      0.f, 0.f, 0.f, 1.f,
    };
    m = glm::scale( m, glm::tvec3< float >( s[ 0 ], s[ 1 ], s[ 2 ] ) );
    auto rot = glm::tquat< float >( r[ 3 ], r[ 0 ], r[ 1 ], r[ 2 ] );
    auto rotm = glm::transpose( glm::mat4_cast( rot ) );
    m = m * rotm;
    m = glm::translate( m, glm::tvec3< float >( t[ 0 ], t[ 1 ], t[ 2 ] ) );
    return ( trans * rotm ) * scale;
  }
  glm::tmat4x4< float > to_matrix(
    const std::array< float, 16 > &t
  ) {
    return glm::tmat4x4< float >(
      t[ 0 ], t[ 1 ], t[ 2 ], t[ 3 ],
      t[ 4 ], t[ 5 ], t[ 6 ], t[ 7 ],
      t[ 8 ], t[ 9 ], t[ 10 ], t[ 11 ],
      t[ 12 ], t[ 13 ], t[ 14 ], t[ 15 ]
    );
  }
}
