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
  auto image = context.device->createImageUnique(
    vk::ImageCreateInfo()
      .setImageType( vk::ImageType::e2D )
      .setFormat( vk::Format::eR8G8B8A8Unorm )
      .setExtent( { 512, 512, 1 } )
      .setMipLevels( 9 )
      .setArrayLayers( 1 )
      .setSamples( vk::SampleCountFlagBits::e1 )
      .setTiling( vk::ImageTiling::eOptimal )
      .setUsage(
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled
      )
      .setSharingMode( vk::SharingMode::eExclusive )
      .setQueueFamilyIndexCount( 0 )
      .setPQueueFamilyIndices( nullptr )
      .setInitialLayout( vk::ImageLayout::eUndefined )
  );
  auto memory_requirements = context.device->getImageMemoryRequirements( *image );
  const auto memory_props = vk::MemoryPropertyFlagBits::eDeviceLocal;
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
  context.device->bindImageMemory( *image, *memory, 0 );
}

