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
  VkImageCreateInfo image_create_info;
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.pNext = nullptr;
  image_create_info.flags = 0;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  image_create_info.extent.width = 512;
  image_create_info.extent.height = 512;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 9;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage =
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    VK_IMAGE_USAGE_SAMPLED_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.queueFamilyIndexCount = 0;
  image_create_info.pQueueFamilyIndices = nullptr;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImage image;
  if( vkCreateImage( *context.device, &image_create_info, nullptr, &image ) != VK_SUCCESS ) {
    std::cout << "イメージを作れない" << std::endl;
    return 1;
  }
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements( *context.device, image, &memory_requirements );
  const auto memory_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
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
    vkDestroyImage( *context.device, image, nullptr );
    return 1;
  }
  if( vkBindImageMemory( *context.device, image, memory, 0 ) != VK_SUCCESS ) {
    std::cout << "イメージにメモリを割り当てる事ができない" << std::endl;
    vkFreeMemory( *context.device, memory, nullptr ); 
    vkDestroyImage( *context.device, image, nullptr );
    return 1;
  }
  vkFreeMemory( *context.device, memory, nullptr ); 
  vkDestroyImage( *context.device, image, nullptr );
}

