#include <vector>
#include <vulkan/vulkan.h>
#include <vw/config.h>
#include <vw/glfw.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  VkApplicationInfo application_info;
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pNext = nullptr;
  application_info.pApplicationName = configs.prog_name.c_str();
  application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.pEngineName ="sample_engine";
  application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  application_info.apiVersion = VK_API_VERSION_1_1;
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
  VkInstanceCreateInfo create_instance_info;
  create_instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_instance_info.pNext = nullptr;
  create_instance_info.flags = 0;
  create_instance_info.pApplicationInfo = &application_info;
  create_instance_info.enabledLayerCount = layers.size();
  create_instance_info.ppEnabledLayerNames = layers.data();
  create_instance_info.enabledExtensionCount = ext.size();
  create_instance_info.ppEnabledExtensionNames = ext.data();
  VkInstance instance;
  if( vkCreateInstance(
    &create_instance_info,
    nullptr,
    &instance
  ) != VK_SUCCESS ) return -1;
  vkDestroyInstance(
    instance,
    nullptr
  );
}

