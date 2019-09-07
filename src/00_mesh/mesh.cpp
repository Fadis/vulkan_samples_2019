#include <iostream>
#include <common/config.h>
#include <common/mesh.h>

int main( int argc, const char *argv[] ) {
  const auto config = common::parse_configs( argc, argv );
  common::scene mesh( config.mesh_file );
  for( const auto c: mesh.get_cameras() ) {
    std::cout << "cameras: ";
    for( size_t i = 0; i != 4; ++i )
      for( size_t j = 0; j != 4; ++j )
        std::cout << c[ i ][ j ] << " ";
    std::cout << std::endl;
  }
  for( const auto m: mesh.get_matrices() ) {
    std::cout << "matrix: ";
    for( size_t i = 0; i != 4; ++i )
      for( size_t j = 0; j != 4; ++j )
        std::cout << m[ i ][ j ] << " ";
    std::cout << std::endl;
  }
  for( const auto v: mesh.get_vertices() ) {
    std::cout << "vertex: ";
    for( size_t j = 0; j != 3; ++j )
      std::cout << v.position[ j ] << " ";
    std::cout << std::endl;
  }
}

