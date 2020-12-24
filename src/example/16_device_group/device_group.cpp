#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/exceptions.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    configs,
    {},
    {}
  );
  if( configs.list ) {
    vw::list_devices( *instance, configs );
    return 0;
  }
  std::vector< const char* > dext{
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
  };
  std::vector< const char* > dlayers{};
  if( !vw::is_capable(
    *instance,
    configs,
    dext,
    dlayers
  ) ) {
    std::cout << "指定された条件に合うデバイスは無かった" << std::endl;
    return 0;
  }
  const auto groups = instance->enumeratePhysicalDeviceGroups();
  for( const auto &g: groups ) {
    for( uint32_t i = 0; i != g.physicalDeviceCount; ++i ) {
      const auto props = g.physicalDevices[ i ].getProperties();
      std::cout << props.deviceName;
      if( i != g.physicalDeviceCount - 1 )
        std::cout << " と ";
      else
        std::cout << " でデバイスグループを組める" << std::endl;
    }
  }
}

