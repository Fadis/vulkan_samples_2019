#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
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
  vw::context_t context;
  create_surface( context, *instance, configs, dext, dlayers );
  create_device( context, dext, dlayers );

  auto memory_properties = context.physical_device.getMemoryProperties();
  const size_t buffer_size = 60;
  auto buffer = context.device->createBufferUnique(
    vk::BufferCreateInfo()
      .setSize( buffer_size )
      .setUsage( vk::BufferUsageFlagBits::eUniformBuffer )
  );
  auto memory_requirements = context.device->getBufferMemoryRequirements( *buffer );
  const auto memory_props = vk::MemoryPropertyFlagBits::eHostVisible;
  auto index = 0;
  const auto memory_type_index = std::distance(
    memory_properties.memoryTypes + 0,
    std::find_if(
      memory_properties.memoryTypes + 0,
      memory_properties.memoryTypes + VK_MAX_MEMORY_TYPES,
      [&]( const auto &v ) {
        return
          ( ( 1 << index++ ) & memory_requirements.memoryTypeBits ) &&
	  ( v.propertyFlags & memory_props ) == memory_props;
      }
    )
  );
  auto memory = context.device->allocateMemoryUnique(
    vk::MemoryAllocateInfo()
      .setAllocationSize( memory_requirements.size )
      .setMemoryTypeIndex( memory_type_index )
  );
  context.device->bindBufferMemory( *buffer, *memory, 0 );
  void *addr = context.device->mapMemory( *memory, 0, buffer_size );
  std::memset( addr, 0, buffer_size );
  context.device->unmapMemory( *memory );
}

