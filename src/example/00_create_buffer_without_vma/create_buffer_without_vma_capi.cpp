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

  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties( context.physical_device, &memory_properties );
  const size_t buffer_size = 60;
  VkBufferCreateInfo buffer_create_info;
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = nullptr;
  buffer_create_info.flags = 0;
  buffer_create_info.size = buffer_size;
  buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_create_info.queueFamilyIndexCount = 0;
  buffer_create_info.pQueueFamilyIndices = nullptr;
  VkBuffer buffer;
  if( vkCreateBuffer( *context.device, &buffer_create_info, nullptr, &buffer ) != VK_SUCCESS ) {
    std::cout << "バッファを作れない" << std::endl;
    return 1;
  }
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements( *context.device, buffer, &memory_requirements );
  const auto memory_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  auto index = 0;
  const auto memory_type_index = std::distance(
    memory_properties.memoryTypes,
    std::find_if(
      memory_properties.memoryTypes,
      memory_properties.memoryTypes + VK_MAX_MEMORY_TYPES,
      [&]( const auto &v ) {
        return
          ( ( 1 << index++ ) & memory_requirements.memoryTypeBits ) &&
	  ( v.propertyFlags & memory_props ) == memory_props;
      }
    )
  );
  VkMemoryAllocateInfo memory_allocation_info;
  memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memory_allocation_info.pNext = nullptr;
  memory_allocation_info.allocationSize = memory_requirements.size;
  memory_allocation_info.memoryTypeIndex = memory_type_index;
  VkDeviceMemory memory;
  if( vkAllocateMemory( *context.device, &memory_allocation_info, nullptr, &memory ) != VK_SUCCESS ) {
    std::cout << "メモリを確保できない" << std::endl;
    vkDestroyBuffer( *context.device, buffer, nullptr );
    return 1;
  }
  if( vkBindBufferMemory( *context.device, buffer, memory, 0 ) != VK_SUCCESS ) {
    std::cout << "バッファにメモリを割り当てる事ができない" << std::endl;
    vkFreeMemory( *context.device, memory, nullptr ); 
    vkDestroyBuffer( *context.device, buffer, nullptr );
    return 1;
  }
  void *addr = nullptr;
  if( vkMapMemory( *context.device, memory, 0, buffer_size, VkMemoryMapFlags( 0 ), &addr ) != VK_SUCCESS ) {
    std::cout << "メモリをCPUから読み書き可能に出来ない" << std::endl;
    vkFreeMemory( *context.device, memory, nullptr ); 
    vkDestroyBuffer( *context.device, buffer, nullptr );
    return 1;
  }
  std::memset( addr, 0, buffer_size );
  vkUnmapMemory( *context.device, memory );
  vkFreeMemory( *context.device, memory, nullptr ); 
  vkDestroyBuffer( *context.device, buffer, nullptr );
}

