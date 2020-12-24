#include <vector>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/glfw.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  std::vector< const char* > ext;
  if( !configs.direct ) {
    vw::glfw::init();
    uint32_t count = 0u;
    auto exts = glfwGetRequiredInstanceExtensions( &count );
    for( uint32_t i = 0u; i != count; ++i )
      ext.push_back( exts[ i ] );
  }
  else {
    ext.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
    ext.push_back( VK_KHR_DISPLAY_EXTENSION_NAME );
    ext.push_back( VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME );
    ext.push_back( VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME );
  }
  std::vector< const char* > layers;
  if( configs.validation ) layers.emplace_back( "VK_LAYER_KHRONOS_validation" );
  const auto app_info = vk::ApplicationInfo(
    configs.prog_name.c_str(), VK_MAKE_VERSION(1, 0, 0),
    "sample_engine", VK_MAKE_VERSION(1, 0, 0),
    VK_API_VERSION_1_2
  );
  auto instance = vk::createInstanceUnique(
    vk::InstanceCreateInfo()
      .setPApplicationInfo( &app_info )
      .setEnabledExtensionCount( ext.size() )
      .setPpEnabledExtensionNames( ext.data() )
      .setEnabledLayerCount( layers.size() )
      .setPpEnabledLayerNames( layers.data() )
  );
}

