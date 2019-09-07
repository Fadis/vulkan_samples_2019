#ifndef VULKAN_SAMPLES_2019_INCLUDE_COMMON_CONFIG_H
#define VULKAN_SAMPLES_2019_INCLUDE_COMMON_CONFIG_H
#include <string>
#include "common/setter.h"
namespace common {
  struct configs_t {
    configs_t() : list( false ), device_index( 0 ), width( 0 ), height( 0 ), fullscreen( false ), validation( false ), debug_mode( false ) {}
    VULKAN_SAMPLE_SET_LARGE_VALUE( prog_name )
    VULKAN_SAMPLE_SET_SMALL_VALUE( list )
    VULKAN_SAMPLE_SET_SMALL_VALUE( device_index )
    VULKAN_SAMPLE_SET_SMALL_VALUE( width )
    VULKAN_SAMPLE_SET_SMALL_VALUE( height )
    VULKAN_SAMPLE_SET_SMALL_VALUE( fullscreen )
    VULKAN_SAMPLE_SET_SMALL_VALUE( validation )
    VULKAN_SAMPLE_SET_SMALL_VALUE( debug_mode )
    VULKAN_SAMPLE_SET_LARGE_VALUE( shader_dir )
    VULKAN_SAMPLE_SET_LARGE_VALUE( mesh_file )
    std::string prog_name; 
    bool list;
    unsigned int device_index;
    unsigned int width;
    unsigned int height;
    bool fullscreen;
    bool validation;
    bool debug_mode;
    std::string shader_dir;
    std::string mesh_file;
  };
  configs_t parse_configs( int argc, const char *argv[] );
}
#endif
