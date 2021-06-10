#include <viewer/light.h>
namespace viewer {
  point_lights_t create_point_light(
    const fx::gltf::Document &doc
  ) {
    point_lights_t lights;
    if( doc.extensionsAndExtras.is_null() ) return point_lights_t();
    if( doc.extensionsAndExtras.find( "extensions" ) == doc.extensionsAndExtras.end() ) return point_lights_t();
    auto &exts = doc.extensionsAndExtras[ "extensions" ];
    if( exts.find( "KHR_lights_punctual" ) == exts.end() ) return point_lights_t();
    auto &ext = exts[ "KHR_lights_punctual" ];
    if( ext.find( "lights" ) == ext.end() ) return point_lights_t();
    for( const auto &l : ext[ "lights" ] ) {
      if( l.find( "type" ) != l.end() && l[ "type" ] == "point" ) {
        point_light_t light;
        if(
          l.find( "color" ) != l.end() &&
          l[ "color" ].is_array() &&
          l[ "color" ].size() == 3u
        ) light.set_color( glm::vec3( l[ "color" ][ 0 ], l[ "color" ][ 1 ], l[ "color" ][ 2 ] ) );
        else light.set_color( glm::vec3( 1.f, 1.f, 1.f ) );
        if(
          l.find( "intensity" ) != l.end() &&
          l[ "intensity" ].is_number()
        ) light.set_intensity( float( l[ "intensity" ] ) );
        else light.set_intensity( 5.f );
        light.set_location( glm::vec3( 0.f, 0.f, 0.f ) );
        lights.emplace_back( std::move( light ) );
      }
    }
    return lights;
  }
}

