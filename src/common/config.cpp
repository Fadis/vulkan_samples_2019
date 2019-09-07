#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/spirit/include/qi.hpp>
#include <cstdlib>
#include "common/config.h"
namespace common { 
  configs_t parse_configs( int argc, const char *argv[] ) {
    namespace po = boost::program_options;
    po::options_description desc( "Options" );
    unsigned int device_index = 0u;
    std::string window_size;
    std::string shader_dir;
    std::string mesh_file;
    desc.add_options()
      ( "help,h", "show this message" )
      ( "list,l", "show all available devices" )
      ( "device,d", po::value< unsigned int >(&device_index)->default_value( 0u ), "use specific device" )
      ( "validation,v", "use VK_LAYER_LUNARG_standard_validation" )
      ( "window,w", po::value< std::string >(&window_size)->default_value( "640x480" ), "window size" )
      ( "fullscreen,f", "fullscreen" )
      ( "debug,g", "debug mode" )
      ( "shader,s",  po::value< std::string >(&shader_dir)->default_value( "../shaders/" ), "shader directory" )
      ( "mesh,m",  po::value< std::string >(&mesh_file)->default_value( "../mesh/sponza.dae" ), "mesh file" );
    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );
    if( vm.count( "help" ) ) {
      std::cout << desc << std::endl;
      exit( 0 );
    }
    namespace qi = boost::spirit::qi;
    auto iter = window_size.begin();
    const auto end = window_size.end();
    boost::fusion::vector< unsigned int, unsigned int > parsed_window_size;
    if( !qi::parse( iter, end, qi::uint_ >> 'x' >> qi::uint_, parsed_window_size ) ) {
      std::cerr << "不正なウィンドウサイズ: " << window_size << std::endl;
      exit( 1 );
    }
    return configs_t()
      .set_prog_name( boost::filesystem::path( argv[ 0 ] ).filename().native() )
      .set_device_index( device_index )
      .set_list( vm.count( "list" ) )
      .set_validation( vm.count( "validation" ) )
      .set_width( boost::fusion::at_c< 0 >( parsed_window_size ) )
      .set_height( boost::fusion::at_c< 1 >( parsed_window_size ) )
      .set_fullscreen( vm.count( "fullscreen" ) )
      .set_debug_mode( vm.count( "debug" ) )
      .set_shader_dir( shader_dir )
      .set_mesh_file( mesh_file );
  }
}

